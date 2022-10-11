
#include "os.h"
#include "problems.h"
#include "rng.h"
#include "interpreter.h"

#include <string>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <cassert>


#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif


using namespace std;


////////////////////////////////////////////////////////////
// objective functions
//


std::map<std::string, ExpressionPtr> lookupObjectiveFunction;
ExpressionPtr getObjectiveFunction(std::string const& name)
{
	std::map<std::string, ExpressionPtr>::const_iterator it = lookupObjectiveFunction.find(name);
	if (it == lookupObjectiveFunction.end()) return ExpressionPtr();
	else return it->second;
}

void compileFunctions(Json dict)
{
	if (!lookupObjectiveFunction.empty()) return;

	for (Json::object_iterator it = dict.object_begin(); it != dict.object_end(); ++it)
	{
		string name = it->first;
		string command = it->second.asString();
		try
		{
			ExpressionPtr problem = parse(command);
			lookupObjectiveFunction[name] = problem;
		}
		catch (exception const& ex)
		{
			string msg = "error while compiling function '" + name + "': " + ex.what();
			throw runtime_error(msg);
		}
	}
}


////////////////////////////////////////////////////////////
// point transformations
//


// transformations of points (inputs)
class PointTransformation
{
public:
	PointTransformation()
	{ }
	virtual ~PointTransformation()
	{ }

	virtual Vector apply(Vector const& x) const = 0;

	Vector operator () (Vector const& x) const
	{
		return apply(x);
	}
};


class VectorIdentity : public PointTransformation
{
public:
	VectorIdentity()
	{ }

	Vector apply(Vector const& x) const
	{
		return x;
	}
};


// uniformly selected rotation matrix
class ShiftAndRotate : public PointTransformation
{
public:
	ShiftAndRotate(long seed, unsigned int dim)
		: m_shift(dim)
		, m_rotation(dim * dim)
	{
		RNG rng(seed);
		for (unsigned int i = 0; i<dim; i++) m_shift[i] = 1.0 * rng.uniform() - 0.5;

		// Note: We now have a proper matrix class and a random number
		// generator function for this. Still, DON'T CHANGE THIS CODE,
		// it must remain intact to reproduce the 2015 competitions!
		for (unsigned int i = 0; i<dim*dim; i++) m_rotation[i] = rng.gauss();
		for (unsigned int i = 0; i<dim; i++)
		{
			double* row_i = &m_rotation[i * dim];
			for (unsigned int j = 0; j<i; j++)
			{
				double* row_j = &m_rotation[j * dim];
				double ip = 0.0;
				for (unsigned int k = 0; k<dim; k++) ip += row_i[k] * row_j[k];
				for (unsigned int k = 0; k<dim; k++) row_i[k] -= ip * row_j[k];
			}
			double norm2 = 0.0;
			for (unsigned int k = 0; k<dim; k++) norm2 += row_i[k] * row_i[k];
			double norm = sqrt(norm2);
			for (unsigned int k = 0; k<dim; k++) row_i[k] /= norm;
		}
	}

	Vector apply(Vector const& x) const
	{
		size_t dim = x.size();
		Vector result(dim);
		for (size_t i = 0; i<dim; i++)
		{
			double v = m_shift[i];
			const double* row = &m_rotation[dim * i];
			for (size_t j = 0; j<dim; j++) v += row[j] * (2.5*(x[j] - 0.5)); //multiply to make sure the optimum is in the feasible region
			result[i] = v + 0.5;
		}

		return result;
	}

protected:
	vector<double> m_shift;
	vector<double> m_rotation;
};


// uniformly selected rotation matrix
class Shift : public PointTransformation
{
public:
	Shift(long seed, unsigned int dim)
		: m_shift(dim)
	{
		RNG rng(seed);
		for (unsigned int i = 0; i<dim; i++) m_shift[i] = 1.0 * rng.uniform() - 0.5;
	}

	Vector apply(Vector const& x) const
	{
		size_t dim = x.size();
		Vector result(dim);
		for (size_t i = 0; i<dim; i++)
			result[i] = m_shift[i] + x[i];

		return result;
	}

protected:
	vector<double> m_shift;
};


// linear time sparse rotation (product of 2N Givens rotations)
class ShiftAndRotateSparse : public PointTransformation
{
public:
	ShiftAndRotateSparse(long seed, unsigned int dim)
		: m_shift(dim)
		, m_axis1(2 * dim)
		, m_axis2(2 * dim)
		, m_sin(2 * dim)
		, m_cos(2 * dim)
	{
		RNG rng(seed);
		for (unsigned int i = 0; i<dim; i++) m_shift[i] = 1.0 * rng.uniform() - 0.5;
		for (unsigned int i = 0; i<2 * dim; i++)
		{
			m_axis1[i] = rng.discrete(0, dim - 1);
			m_axis2[i] = rng.discrete(0, dim - 2);
			if (m_axis2[i] >= m_axis1[i]) m_axis2[i]++;
			double angle = 2.0 * M_PI * rng.uniform();
			m_sin[i] = sin(angle);
			m_cos[i] = cos(angle);
		}
	}

	Vector apply(Vector const& x) const
	{
		size_t dim = x.size();
		Vector result(dim);
		for (size_t i = 0; i<dim; i++) result[i] = 2.5*(x[i] - 0.5);	//multiply to make sure the optimum is in the feasible region
		for (size_t i = 0; i<2 * dim; i++)
		{
			double a = result[m_axis1[i]];
			double b = result[m_axis2[i]];
			double x = m_cos[i] * a - m_sin[i] * b;
			double y = m_sin[i] * a + m_cos[i] * b;
			result[m_axis1[i]] = x;
			result[m_axis2[i]] = y;
		}
		for (size_t i = 0; i<dim; i++) result[i] += m_shift[i] + 0.5;
		return result;
	}

protected:
	vector<double> m_shift;
	vector<unsigned int> m_axis1;
	vector<unsigned int> m_axis2;
	vector<double> m_sin;
	vector<double> m_cos;
};


PointTransformation* createPointTransformation(string const& name, long seed, unsigned int dimension)
{
	if (name == "Identity") return new VectorIdentity();
	else if (name == "Shift") return new Shift(seed, dimension);
	else if (name == "ShiftAndRotate") return new ShiftAndRotate(seed, dimension);
	else if (name == "ShiftAndRotateSparse") return new ShiftAndRotateSparse(seed, dimension);
	else throw runtime_error("unknown point transformation: " + name);
}


////////////////////////////////////////////////////////////
// value transformations
//


// transformations of values (outputs, only for scalars)
class ValueTransformation
{
public:
	ValueTransformation()
	{ }
	virtual ~ValueTransformation()
	{ }

	virtual double apply(double value) const = 0;

	double operator () (double value) const
	{
		return apply(value);
	}
};


class ValueIdentity : public ValueTransformation
{
public:
	ValueIdentity()
	{ }

	double apply(double value) const
	{
		return value;
	}
};


class Tanh : public ValueTransformation
{
public:
	Tanh()
	{ }

	double apply(double value) const
	{
		return tanh(value);
	}
};

class AbsPow05 : public ValueTransformation
{
public:
	AbsPow05()
	{ }

	double apply(double value) const
	{
		return std::pow(std::fabs(value), 0.5);
	}
};

class Steps : public ValueTransformation
{
public:
	Steps(long seed)
		: m_pos(100)
		, m_value(101)
	{
		RNG rng(seed);
		for (unsigned int i = 0; i<100; i++) m_pos[i] = exp(-20.0 * rng.uniform());
		for (unsigned int i = 0; i<101; i++) m_value[i] = (i == 0) ? 0.0 : m_value[i - 1] + rng.uniform();
		sort(m_pos.begin(), m_pos.end());
	}

	double apply(double value) const
	{
		vector<double>::const_iterator it = upper_bound(m_pos.begin(), m_pos.end(), value);
		unsigned int index = distance(m_pos.begin(), it);
		if (index > 100) index = 100;
		return (value + m_value[index]);
	}

protected:
	vector<double> m_pos;
	vector<double> m_value;
};


class Splines : public ValueTransformation
{
public:
	Splines(long seed)
		: m_pos(100)
	{
		RNG rng(seed);
		for (unsigned int i = 0; i<100; i++) m_pos[i] = exp(-20.0 * rng.uniform());
		sort(m_pos.begin(), m_pos.end());
	}

	double apply(double value) const
	{
		if (value <= m_pos[0]) return value;
		if (value >= m_pos[99]) return value;
		vector<double>::const_iterator it = upper_bound(m_pos.begin(), m_pos.end(), value);
		unsigned int index = distance(m_pos.begin(), it);
		if (index == 0) index = 1;
		double v0 = m_pos[index - 1];
		double v1 = m_pos[index];
		double x = (value - v0) / (v1 - v0);
		assert(x >= 0.0 && x <= 1.0);
		return (v0 + ((2.25 - 1.5 * x) * x + 0.25) * x);
	}

protected:
	vector<double> m_pos;
};

class NormalizedLogMin10 : public ValueTransformation
{
public:
	NormalizedLogMin10()
	{ }

	double apply(double value) const
	{	// f in [0,+inf] -> max(log10(f),log10(1e-10)) / (log10(1e+10) - log10(1e-10)) which is in [0,k] and max k is in order of 1
		if (value < 0)
		{
			// printf("NormalizedLogMin10: value %g is set to 1e-10\n", value);
			value = 1e-10;
		}
		value = log10(value);
		double minlogval = -10;
		double approxmaxlogval = 10;
		if (value < minlogval)	value = minlogval;
		value = (value - minlogval) / (approxmaxlogval - minlogval);
		value = 1 * pow(value, 4.0);
		return value;
	}
};

ValueTransformation* createValueTransformation(string const& name, long seed)
{
	if (name == "Identity") return new ValueIdentity();
	else if (name == "Tanh") return new Tanh();
	else if (name == "Steps") return new Steps(seed);
	else if (name == "Splines") return new Splines(seed);
	else if (name == "AbsPow05") return new AbsPow05();
	else if (name == "NormalizedLogMin10") return new NormalizedLogMin10();
	else throw runtime_error("unknown value transformation: " + name);
}


////////////////////////////////////////////////////////////
// evaluation code
//

// A problem (objective), composed of components and objectives,
// as well as point and value transformations
class Problem1 : public Problem
{
public:
	// create a problem instance from a json object
	Problem1(Json definition);
	~Problem1();

	double evalSO(Vector const& x) const;
	Vector evalMO(Vector const& x) const;

private:
	// component (inner function)
	struct Component
	{
		Component();
		Component(Json definition, int& seed);
		~Component();

		double eval(Vector const& x) const;

		unsigned int dimension;                     // input dimensionality of the component
		PointTransformation* pointTransformation;   // can be nullptr
		ExpressionPtr function;                     // component function
		ValueTransformation* valueTransformation;   // can be nullptr
	};

	// objective function (function of the component outputs)
	struct Objective
	{
		Objective();
		Objective(Json definition, int& seed);
		~Objective();

		double eval(Vector const& x) const;

		ExpressionPtr function;                     // objective function
		ValueTransformation* valueTransformation;   // can be nullptr
	};

	// problem data
	PointTransformation* m_globalPointTransformation;              // global input transformation, can be nullptr
	std::vector<Component*> m_component;                           // component functions
	std::vector<Objective*> m_objective;                           // one function per objective
};

Problem1::Problem1(Json definition)
	: m_globalPointTransformation(nullptr)
{
	int seed = (int)definition["seed"].asNumber();

	m_dimension = (unsigned int)definition["dimension"].asNumber();
	int curseed = seed;
	if (definition.has("components"))
	{
		if (definition.has("version"))
		{
			if (definition["version"].asString() == "2015")
			{
				////////////////////////////////////////////////////////////
				// BBComp'2015 style of problem definition
				//
				if (definition.has("inputTrans"))
					m_globalPointTransformation = createPointTransformation(definition["inputTrans"], curseed++, m_dimension);

				Json jcomp = definition["components"];
				for (size_t i = 0; i < jcomp.size(); i++)
				{
					m_component.push_back(new Component());
					m_component[i]->dimension = (unsigned int)jcomp[i]["dimension"].asNumber();
					m_component[i]->function = getObjectiveFunction(jcomp[i]["function"].asString());
					if (jcomp[i].has("inputTrans")) m_component[i]->pointTransformation = createPointTransformation(jcomp[i]["inputTrans"], curseed++, m_component[i]->dimension);
				}
				for (size_t i = 0; i < jcomp.size(); i++)
					if (jcomp[i].has("valueTrans")) m_component[i]->valueTransformation = createValueTransformation(jcomp[i]["valueTrans"], curseed++);

				Json jobj = definition["objectives"];
				size_t nObj = jobj.isArray() ? jobj.size() : 1;
				if (jobj.isArray())
				{
					for (size_t i = 0; i < nObj; i++)	m_objective.push_back(new Objective(jobj[i], curseed));
				}
				else
				{
					m_objective.push_back(new Objective(jobj, curseed));
				}
			}
			else throw runtime_error("[Problem1::Problem1] unknown version");
		}
		else
		{
			////////////////////////////////////////////////////////////
			// new style problem definition
			//

			if (definition.has("inputTrans"))
			{
				m_globalPointTransformation = createPointTransformation(definition["inputTrans"], curseed++, m_dimension);
			}
			Json jcomp = definition["components"];
			for (size_t i = 0; i < jcomp.size(); i++)
			{
				m_component.push_back(new Component(jcomp[i], curseed));
			}
			Json jobj = definition["objectives"];
			size_t nObj = jobj.isArray() ? jobj.size() : 1;
			if (jobj.isArray())
			{
				for (size_t i = 0; i < nObj; i++)	m_objective.push_back(new Objective(jobj[i], curseed));
			}
			else
			{
				m_objective.push_back(new Objective(jobj, curseed));
			}
		}
	}

	m_objectives = m_objective.size();
}

Problem1::~Problem1()
{
	if (m_globalPointTransformation) { delete m_globalPointTransformation;	m_globalPointTransformation = nullptr; }
	for (size_t i = 0; i<m_component.size(); i++) delete m_component[i];
	for (size_t i = 0; i<m_objective.size(); i++) delete m_objective[i];
}


double Problem1::evalSO(Vector const& x) const
{
	assert(x.size() == dimension());
	assert(objectives() == 1);

	// global transformation
	Vector xx = (m_globalPointTransformation) ? (*m_globalPointTransformation)(x) : x;

	// component operations
	Vector intermediate(m_component.size());
	unsigned int start = 0;
	for (size_t i = 0; i<m_component.size(); i++)
	{
		unsigned int dim = m_component[i]->dimension;
		intermediate[i] = m_component[i]->eval(xx.sub(start, start + dim));
		start += dim;
	}
	assert(start == dimension());

	// value operations
	double fx = m_objective[0]->eval(intermediate);

	// never return INF/NaN
	if (!std::isfinite(fx)) fx = 1e99;

	// return objective value
	return fx;
}

Vector Problem1::evalMO(Vector const& x) const
{
	assert(x.size() == dimension());
	assert(objectives() > 1);

	// global transformation
	Vector xx = (m_globalPointTransformation) ? (*m_globalPointTransformation)(x) : x;

	// component operations
	Vector intermediate(m_component.size());
	unsigned int start = 0;
	for (size_t i = 0; i<m_component.size(); i++)
	{
		unsigned int dim = m_component[i]->dimension;
		intermediate[i] = m_component[i]->eval(xx.sub(start, start + dim));
		start += dim;
	}
	assert(start == dimension());

	// value operations
	Vector fx((size_t)objectives());
	for (size_t i = 0; i<objectives(); i++)
	{
		fx[i] = m_objective[i]->eval(intermediate);
	}

	// Apply component-wise sigmoid.
	// I am not using the "standard" logistic sigmoid here because it
	// saturates too quickly (exponentially fast). Also, the input
	// value is scaled for better resolution in the "relevant" range.
	// Negative values (which *should* never occur) are truncated.
	for (size_t i = 0; i<objectives(); i++)
	{
		double v = 0.01 * fx[i];
		fx[i] = (v <= 0.0) ? 0.0 : v / sqrt(1.0 + v * v);
	}

	// never return INF/NaN
	for (size_t i = 0; i<objectives(); i++) if (!std::isfinite(fx[i])) fx[i] = 1e99;

	// return objective vector
	return fx;
}


Problem1::Component::Component()
	: dimension(0)
	, pointTransformation(nullptr)
	, valueTransformation(nullptr)
{ }

Problem1::Component::Component(Json definition, int& seed)
	: dimension(0)
	, pointTransformation(nullptr)
	, valueTransformation(nullptr)
{
	dimension = (unsigned int)definition["dimension"].asNumber();
	function = getObjectiveFunction(definition["function"].asString());
	if (definition.has("inputTrans")) pointTransformation = createPointTransformation(definition["inputTrans"], seed++, dimension);
	if (definition.has("valueTrans")) valueTransformation = createValueTransformation(definition["valueTrans"], seed++);
}

Problem1::Component::~Component()
{
	if (pointTransformation) { delete pointTransformation;		pointTransformation = nullptr; }
	if (valueTransformation) { delete valueTransformation;		valueTransformation = nullptr; }
}

double Problem1::Component::eval(Vector const& x) const
{
	assert(x.size() == dimension);
	Vector xx = (pointTransformation) ? (*pointTransformation)(x) : x;
	double fx = evaluate(function, xx);
	double ret = (valueTransformation) ? (*valueTransformation)(fx) : fx;
	return ret;
}


Problem1::Objective::Objective()
	: valueTransformation(nullptr)
{ }

Problem1::Objective::Objective(Json definition, int& seed)
	: valueTransformation(nullptr)
{
	string fname = definition["function"].asString();
	if (fname != "Identity") function = getObjectiveFunction(fname);
	if (definition.has("valueTrans")) valueTransformation = createValueTransformation(definition["valueTrans"], seed++);
}

Problem1::Objective::~Objective()
{
	if (valueTransformation) delete valueTransformation;
}

double Problem1::Objective::eval(Vector const& x) const
{
	double fx;
	if (function)
	{
		Vector xx(x);
		for (size_t i = 0; i<xx.size(); i++)
			xx[i] = xx[i] + 0.5;   // optimum at (1/2, ..., 1/2)
		fx = evaluate(function, xx);
	}
	else
	{
		assert(x.size() == 1);
		fx = x[0];
	}
	return (valueTransformation) ? (*valueTransformation)(fx) : fx;
}


////////////////////////////////////////////////////////////


// The Problem2MO class represents MO problems used for the 2016
// competition.
class Problem2MO : public Problem
{
public:
	// create a problem instance from a json object
	Problem2MO(Json definition)
	{
		Json jObjectives = definition["objectives"];
		Json jObjectiveCoop = definition["objective-coop"];
		m_dimension = (unsigned int)definition["dimension"].asNumber();
		m_cooperative = (unsigned int)definition["cooperative"].asNumber();
		m_competitive = (unsigned int)definition["competitive"].asNumber();
		m_objectives = jObjectives.size();

		if ((m_dimension <= 1) || (m_dimension != m_cooperative + m_competitive) || (m_objectives != 2 && m_objectives != 3))
		{
			throw runtime_error("[Problem2MO::Problem2MO] invalid parameters");
		}

		RNG rng((unsigned int)definition["seed"].asNumber());

		m_target.resize(m_objectives);
		m_mo.resize(m_objectives);

		// target positions
		for (size_t j = 0; j<m_objectives; j++)
		{
			m_target[j] = jObjectives[j]["target"].asNumberArray();
			if (m_target[j].size() != m_dimension) throw runtime_error("[Problem2MO::Problem2MO] target position dimension mismatch");
		}

		// feature map / transformation
		m_featuremap = FeatureMap(rng, definition["transformation"], m_dimension, m_cooperative, m_target);

		// objective functions
		m_so = TransformedObjective(rng, Vector(m_cooperative, 0.0), jObjectiveCoop);
		for (size_t j = 0; j<m_objectives; j++)
		{
			Vector opt = m_featuremap(m_target[j]).sub(m_cooperative, m_dimension, false);
			m_mo[j] = TransformedObjective(rng, opt, jObjectives[j]);
		}

		// front shaping exponent
		m_shaping = definition["front-shaping"].asNumber();
	}

	~Problem2MO()
	{ }


	double evalSO(Vector const& x) const
	{
		// This problem class is defined only for multi-objective problems.
		throw runtime_error("[Problem2MO::evalSO] internal error");
	}

	Vector evalMO(Vector const& x) const
	{
		assert(m_mo.size() == m_objectives);
		Vector Tx = m_featuremap(x);
		Vector so_x = Tx.sub(0, m_cooperative, false);
		Vector mo_x = Tx.sub(m_cooperative, m_dimension, false);
		double f = m_so(so_x);
		Vector ret(m_objectives, 0.0);
		for (unsigned int j = 0; j<m_objectives; j++)
		{
			ret(j) = pow(squash(f + m_mo[j](mo_x)), m_shaping);
			if (ret(j) < 0.0 || ret(j) > 1.0 || !std::isfinite(ret(j))) ret(j) = 1.0;
		}
		return ret;
	}

private:
	// squashing function [-\infty, \infty] \to [0, 1]
	static double sigmoid(double t)
	{
		return 1.0 / (1.0 + exp(-t));
	}

	// squashing function [0, \infty] \to [0, 1]
	static double squash(double x)
	{
		if (x <= 0.0) return 0.0;
		else return (1.0 / (1.0 + 1.0 / x));
	}

	// non-linear component of the feature transformation
	struct Distortion
	{
		Distortion() = default;
		Distortion(Distortion const& other) = default;

		Distortion(RNG& rng, unsigned int dimension)
		{
			double z = 0.0;
			switch (rng.discrete(0, 9))
			{
			case 0:
			case 1:
			case 2:
				type = wave;
				s = rng.uniform(0.0, 2.0 * M_PI);
				m = 10.0 / sqrt((double)dimension) * rng.gaussVector(dimension);
				z = pow(0.25, rng.uniform()) / m.twonorm();
				break;
			case 3:
			case 4:
			case 5:
				type = bump;
				s = 0.5 * pow(0.1, 1.0 * rng.uniform());
				m = Vector(dimension, 0.0);
				for (unsigned int k = 0; k<dimension; k++) m[k] = rng.uniform();
				z = s * pow(0.25, rng.uniform());
				break;
			case 6:
			case 7:
			case 8:
				type = ramp;
				s = rng.uniform(-10.0, 10.0);
				m = 10.0 / sqrt((double)dimension) * rng.gaussVector(dimension);
				for (unsigned int k = 0; k<dimension; k++) m[k] = -abs(m[k]);
				z = pow(0.25, rng.uniform()) / m.twonorm();
				break;
			case 9:
				type = cliff;
				s = rng.uniform(-10.0, 10.0);
				m = 10.0 / sqrt((double)dimension) * rng.gaussVector(dimension);
				for (unsigned int k = 0; k<dimension; k++) m[k] = -abs(m[k]);
				z = pow(0.25, rng.uniform()) / m.twonorm();
				break;
			default:
				throw runtime_error("[Distortion::Distortion] internal error");
			}
			v = z * rng.unitVector(dimension);
		}

		// evaluate the distortion function
		Vector operator () (Vector const& x) const
		{
			double f = 0.0;
			switch (type)
			{
			case wave:
				f = cos(m * x + s);
				break;
			case bump:
				f = exp((x - m).twonorm2() / (-2.0 * s * s));
				break;
			case ramp:
				f = sigmoid(m * x + s);
				break;
			case cliff:
				f = (m * x + s >= 0.0) ? 1.0 : 0.0;
				break;
			default:
				throw runtime_error("[Distortion] unknown type");
			}
			return f * v;
		}

		enum Type
		{
			wave,
			bump,
			ramp,
			cliff,
		};

		Type type;                      // distortion type
		double s;                       // scalar parameter (for scaling or offset)
		Vector m;                       // input parameter vector
		Vector v;                       // output parameter vector
	};

	struct FeatureMap
	{
		FeatureMap() = default;
		FeatureMap(FeatureMap const& other) = default;

		FeatureMap(RNG& rng, Json definition, unsigned int dimension, unsigned int cooperative, vector<Vector> const& points)
		{
			string r = definition["rotation"].asString();
			if (r == "none") B = Matrix::identity(dimension);
			else if (r == "random") B = rng.orthogonalMatrix(dimension);
			else throw runtime_error("[Problem2MO::FeatureMap] invalid rotation type");

			unsigned int n = (unsigned int)definition["distortions"].asNumber();
			nonlinear.resize(n);
			for (size_t i = 0; i<n; i++) nonlinear[i] = Distortion(rng, dimension);

			// compensate for distortions in the first #cooperative components at the given points
			size_t m = points.size();
			if (cooperative > 0)
			{
				Matrix X(m, dimension);
				Matrix F(m, dimension, 0.0);
				for (size_t i = 0; i<m; i++)
				{
					if (points[i].size() != dimension) throw runtime_error("[Problem2MO::FeatureMap] dimension mismatch");
					X.row(i) = points[i];
					F.row(i).sub(0, cooperative) = (*this)(points[i]).sub(0, cooperative, false);
				}
				Matrix A = (X.inverse() * F).transpose();
				B -= A;
			}
		}

		// evaluate the feature map
		Vector operator () (Vector const& x) const
		{
			Vector ret = B * x;
			for (unsigned int i = 0; i<nonlinear.size(); i++) ret += nonlinear[i](x);
			return ret;
		}

		Matrix B;                        // linear transformation
		vector<Distortion> nonlinear;    // non-linear distortions
	};

	struct TransformedObjective
	{
		TransformedObjective() = default;
		TransformedObjective(TransformedObjective const& other) = default;

		TransformedObjective(RNG& rng, Vector const& opt, Json definition)
			: optimum(opt)
			, function(getObjectiveFunction(definition["function"].asString()))
			, scaling(definition["scaling"].asNumber())
		{
			if (!function) throw runtime_error("[Problem2MO::TransformedObjective] unknown function");

			unsigned int dimension = opt.size();
			if (dimension > 0)
			{
				string r = definition["rotation"].asString();
				if (r == "none") rotation = Matrix::identity(dimension);
				else if (r == "random") rotation = rng.orthogonalMatrix(dimension);
				else throw runtime_error("[Problem2MO::TransformedObjective] invalid rotation type");
			}
		}

		double operator () (Vector const& x) const
		{
			if (x.size() == 0) return 0.0;
			else return scaling * evaluate(function, rotation * (x - optimum));
		}

		Vector optimum;                 // optimal input
		Matrix rotation;                // rotation of the input vector
		ExpressionPtr function;         // actual objective function
		double scaling;                 // scaling of the output value
	};

	unsigned int m_cooperative;          // number of "cooperative" features
	unsigned int m_competitive;          // number of "competitive" features
	vector<Vector> m_target;             // objective-wise optimum
	FeatureMap m_featuremap;             // non-linear feature map
	TransformedObjective m_so;           // objective function on non-competitive features
	vector<TransformedObjective> m_mo;   // objective functions on competitive features
	double m_shaping;                    // exponent for shaping of the front
};


////////////////////////////////////////////////////////////


Problem* createProblem(Json definition)
{
	// Obtain the class representing the problem, with "Problem1"
	// as a default for backwards compatibility.
	string cls = definition["class"]("Problem1");

	// Instantiate the requested problem sub-class.
	if (cls == "Problem1") return new Problem1(definition);
	else if (cls == "Problem2MO") return new Problem2MO(definition);
	else throw runtime_error("[createProblem] unknown problem class '" + cls + "'");
}
