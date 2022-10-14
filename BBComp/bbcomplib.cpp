
#define _CRT_SECURE_NO_WARNINGS

#include "os.h"
#include "variant.h"
#include "json.h"
#include "vector.h"
#include "matrix.h"
#include "rng.h"
#include "paretofront.h"
#include "hypervolume.h"
#include "interpreter.h"
#include "problems.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <string>
#include <algorithm>
#include <memory>


using namespace std;


#include "bbcomplib.h"


////////////////////////////////////////////////////////////
// problem instance for evaluation
//
struct ProblemInstance
{
	ProblemInstance()
	: m_id(-1)
	, m_budget(0)
	, m_evaluations(0)
	, m_bestvalue(1e100)
	, m_problem(nullptr)
	{ }

	unsigned int dimension() const
	{ return m_problem ? m_problem->dimension() : 0; }
	unsigned int objectives() const
	{ return m_problem ? m_problem->objectives() : 0; }

	void clear()
	{
		m_id = -1;
		m_budget = 0;
		m_evaluations = 0;
		m_bestvalue = 1e100;
		m_nondominated.clear();
		m_problemname = "";

		if (m_problem) { delete m_problem; m_problem = nullptr; }
	}

	bool set(int id, Json definition, int evals)
	{
		clear();
		try
		{
			m_id = id;
			m_budget = (int)definition["budget"].asNumber();
			m_evaluations = evals;
			m_bestvalue = 1e100;
			m_problemname = definition["type"].asString();

			m_problem = createProblem(definition);
			unsigned int dim = m_problem->dimension();
			unsigned int obj = m_problem->objectives();
			m_nondominated.clear();
			if (dim == 0 || obj == 0) { clear(); return false; }

			return true;
		}
		catch (...)
		{
			clear();
			return false;
		}
	}

	double evalSO(const double* point)
	{
		assert(m_problem);
		assert(m_problem->objectives() == 1);

		Vector x((size_t)m_problem->dimension());
		for (size_t i=0; i<x.size(); i++) x[i] = point[i];
		return m_problem->evalSO(x);
	}

	Vector evalMO(const double* point)
	{
		assert(m_problem);
		assert(m_problem->objectives() > 1);

		Vector x((size_t)m_problem->dimension());
		for (size_t i=0; i<x.size(); i++) x[i] = point[i];
		return m_problem->evalMO(x);
	}

	void update(Vector const& value)
	{
		m_evaluations++;
		unsigned int nObj = objectives();
		if (nObj == 1)
		{
			m_bestvalue = min(m_bestvalue, value[0]);
		}
		else
		{
			assert(nObj > 1);
			if (m_nondominated.insert(value))
			{
				Vector refpoint(nObj, 1.0);
				m_bestvalue = 1.0 - hypervolume(refpoint, m_nondominated);
			}
		}
	}

	int m_id;                                              // index within the track
	int m_budget;                                          // maximal number of black-box queries
	int m_evaluations;                                     // current number of black-box queries
	double m_bestvalue;                                    // performance achieved so far (hypervolume in the MO case)
	ParetoFront m_nondominated;                            // MO case: non-dominated points
	string m_problemname;                                  // (pretty useless)
	Problem* m_problem;
};


////////////////////////////////////////////////////////////
// library state
//

// static string buffers
char g_errorMessage[1024] = "";
char g_returnBuffer[1024] = "";

// overall program state
const int stateReady = 1;               // the library has started up
const int stateLoaded = 2;              // problem and track definitions were loaded
const int stateTrackSelected = 3;       // track is selected, ready for setProblem
const int stateProblemSelected = 4;     // problem is selected, ready for optimization!
int g_state = 1;
Json j_tracks;                // array of all tracks

// valid at stateTrackSelected and later:
Json j_track;                 // current track

// valid at stateProblemSelected:
Json j_problem;               // json problem description
ProblemInstance g_problem;    // problem instance


////////////////////////////////////////////////////////////
// plain C language interface,
// suitable for many language bindings
//

#ifdef __cplusplus
extern "C" {
#endif

int loadProblems(stringtype problemfile, stringtype tracksfile)
{
	try
	{
		Json j_problems;
		if (! j_problems.load(problemfile))
		{
			strcpy(g_errorMessage, "failed to load problem definitions");
			return 0;
		}

		if (! j_tracks.load(tracksfile))
		{
			strcpy(g_errorMessage, "failed to load track definitions");
			return 0;
		}

		try
		{
			compileFunctions(j_problems);
		}
		catch (exception const& ex)
		{
			strcpy(g_errorMessage, "error setting up problems in the track");
			return 0;
		}

		g_state = stateLoaded;
		return 1;
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during loadProblems");
		return 0;
	}
}

int numberOfTracks()
{
	try
	{
		if (g_state < stateLoaded)
		{
			strcpy(g_errorMessage, "not ready");
			return 0;
		}
		return j_tracks.size();
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during numberOfTracks");
		return 0;
	}
}

stringtype trackName(int trackindex)
{
	try
	{
		// sanity checks
		if (g_state < stateLoaded)
		{
			strcpy(g_errorMessage, "not ready");
			return 0;
		}
		if (trackindex < 0 || trackindex >= (int)j_tracks.size())
		{
			strcpy(g_errorMessage, "track index out of range");
			return 0;
		}

		// extract the track name
		string s = j_tracks[trackindex]["name"].asString();
		if (s.size() >= 1024)
		{
			strcpy(g_errorMessage, "track name too long (>= 1024 characters)");
			return 0;
		}
		strcpy(g_returnBuffer, s.c_str());
		return g_returnBuffer;
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during trackName");
		return NULL;
	}
}

int setTrack(stringtype trackname)
{
	try
	{
		// sanity check
		if (g_state < stateLoaded)
		{
			strcpy(g_errorMessage, "not ready");
			return 0;
		}

		g_problem.clear();
		g_state = stateLoaded;

		// set the track
		for (size_t i=0; i<j_tracks.size(); i++)
		{
			Json track = j_tracks[i];
			if (track["name"] == trackname)
			{
				j_track = track;
				g_state = stateTrackSelected;
				return 1;
			}
		}
		strcpy(g_errorMessage, "unknown track name: '");
		strncat(g_errorMessage, trackname, sizeof(g_errorMessage) - 50);
		strcat(g_errorMessage, "'");
		return 0;
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during setTrack");
		return 0;
	}
}

int numberOfProblems()
{
	try
	{
		if (g_state < stateTrackSelected)
		{
			strcpy(g_errorMessage, "no track selected");
			return 0;
		}
		return j_track["problems"].size();
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during numberOfProblems");
		return 0;
	}
}

int setProblem(int problemID)
{
	try
	{
		// sanity check
		if (g_state < stateTrackSelected)
		{
			strcpy(g_errorMessage, "no track selected");
			return 0;
		}

		g_problem.clear();
		g_state = stateTrackSelected;

		// check parameter range
		if (problemID < 0 || problemID >= (int)j_track["problems"].size())
		{
			strcpy(g_errorMessage, "track index out of range");
			return 0;
		}
		j_problem = j_track["problems"][problemID];

		// create problem instance
		if (! g_problem.set(problemID, j_problem, 0))
		{
			strcpy(g_errorMessage, "internal error: problem instance creation failed");
			return 0;
		}

		// success
		g_state = stateProblemSelected;
		return 1;
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during setProblem");
		return 0;
	}
}

int dimension()
{
	try
	{
		if (g_state < stateProblemSelected)
		{
			strcpy(g_errorMessage, "no problem selected");
			return 0;
		}
		return g_problem.dimension();
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during dimension");
		return 0;
	}
}

int numberOfObjectives()
{
	try
	{
		if (g_state < stateProblemSelected)
		{
			strcpy(g_errorMessage, "no problem selected");
			return 0;
		}
		return g_problem.objectives();
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during numberOfObjectives");
		return 0;
	}
}

int budget()
{
	try
	{
		if (g_state < stateProblemSelected)
		{
			strcpy(g_errorMessage, "no problem selected");
			return 0;
		}
		return g_problem.m_budget;
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during budget");
		return 0;
	}
}

int evaluations()
{
	try
	{
		if (g_state < stateProblemSelected)
		{
			strcpy(g_errorMessage, "no problem selected");
			return -1;
		}

		return g_problem.m_evaluations;
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during evaluations");
		return -1;
	}
}

int evaluate(double* point, double* value)
{
	try
	{
		// initialize output argument
		for (size_t i=0; i<g_problem.objectives(); i++) value[i] = 1e100;

		// sanity checks
		if (g_state < stateProblemSelected)
		{
			strcpy(g_errorMessage, "no problem selected");
			return 0;
		}
		if (g_problem.m_evaluations >= g_problem.m_budget)
		{
			strcpy(g_errorMessage, "evaluation budget exceeded");
			return 0;
		}

		// check box constraints
		bool good = true;
		for (int i=0; i<(int)g_problem.dimension(); i++)
		{
			if (point[i] < 0.0 || point[i] > 1.0) good = false;
		}
		if (! good)
		{
			strcpy(g_errorMessage, "attempt to evaluate an infeasible point");
			return 0;
		}

		// actual evaluation
		Vector val(g_problem.objectives(), 1e100);
		if (g_problem.objectives() == 1)
		{
			val[0] = g_problem.evalSO(point);
		}
		else
		{
			val = g_problem.evalMO(point);
		}

		// return the value(s)
		for (size_t i=0; i<val.size(); i++) value[i] = val[i];
		g_problem.update(val);

		// success
		return 1;
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during evaluate");
		return 0;
	}
}

double performance()
{
	try
	{
		if (g_state < stateProblemSelected)
		{
			strcpy(g_errorMessage, "no problem selected");
			return 1e100;
		}
		else
		{
			if (g_problem.m_evaluations == 0) strcpy(g_errorMessage, "no evaluations available");
			return g_problem.m_bestvalue;
		}
	}
	catch (...)
	{
		g_problem.clear();
		g_state = stateReady;
		strcpy(g_errorMessage, "unhandled error during performance");
		return 0;
	}
}

stringtype errorMessage()
{
	return g_errorMessage;
}

#ifdef __cplusplus
} // extern "C"
#endif
