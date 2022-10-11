
#pragma once

#include <string>
#include <map>

#include "json.h"
#include "vector.h"


// Abstract base class of all optimization problems.
class Problem
{
public:
	Problem(unsigned int dim = 0, unsigned int obj = 0)
	: m_dimension(dim)
	, m_objectives(obj)
	{ }

	virtual ~Problem()
	{ }

	unsigned int dimension() const
	{ return m_dimension; }
	unsigned int objectives() const
	{ return m_objectives; }

	virtual double evalSO(Vector const& x) const = 0;
	virtual Vector evalMO(Vector const& x) const = 0;

protected:
	unsigned int m_dimension;
	unsigned int m_objectives;
};


// Initialize "elementary" objective functions, call this function
// once for initialization.
void compileFunctions(Json dict);


// The createProblem factory function should be used for creating
// problem objects from descriptions, rather than calling the
// corresponding constructors directly. This allows for extending the
// set of Problem subclasses in a transparent manner.
Problem* createProblem(Json definition);
