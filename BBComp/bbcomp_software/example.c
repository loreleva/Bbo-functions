
#include "bbcomplib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


// This example client program implements random search
// to optimize the first problem in the "trial" track.
void optimize()
{
	if (loadProblems("problems.json", "tracks.json") == 0) { printf("loadProblems() failed: %s\n", errorMessage()); return; }
	printf("successfully loaded problem and track definitions\n");

	// list the tracks available to this user (this is optional)
	int numTracks = numberOfTracks();
	if (numTracks == 0) { printf("numberOfTracks() failed: %s\n", errorMessage()); return; }
	printf("%d track(s):\n", numTracks);
	for (int i=0; i<numTracks; i++)
	{
		const char* trackname = trackName(i);
		if (trackname == NULL) { printf("trackName() failed: %s\n", errorMessage()); return; }
		printf("  %d: %s\n", i, trackname);
	}

	// set the track specified at the top
	if (setTrack("trial") == 0) { printf("setTrack failed: %s\n", errorMessage()); return; }
	printf("selected track 'trial'\n");

	// obtain the number of problems in the track
	int numProblems = numberOfProblems();
	printf("number of problems in the track: %d\n", numProblems);

	// For demonstration purposes we optimize only the first problem in the track.
	int problemID = 0;
	if (setProblem(problemID) == 0) { printf("setProblem failed: %s\n", errorMessage()); return; }
	printf("selected problem: %d\n", problemID);

	// obtain problem properties
	int dim = dimension();
	int obj = numberOfObjectives();
	int bud = budget();
	int evals = evaluations();
	printf("problem dimension: %d\n", dim);
	printf("number of objectives: %d\n", obj);
	printf("problem budget: %d\n", bud);
	printf("number of used up evaluations: %d\n", evals);

	// allocate memory for a search point
	//double* point = (double*)malloc(dim * sizeof(double));
	///if (point == NULL) { printf("out of memory\n"); return; }
	//double* value = (double*)malloc(obj * sizeof(double));
	//if (value == NULL) { printf("out of memory\n"); return; }

	// check that we are indeed done
	evals = evaluations();
	if (evals == bud) printf("optimization finished; final performance: %g\n", performance());
	else printf("something went wrong: number of evaluations does not coincide with budget :(\n");
}

int main()
{
	optimize();
}
