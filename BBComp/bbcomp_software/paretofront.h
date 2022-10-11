
#pragma once


#include "vector.h"
#include <vector>
#include <list>
#include <cmath>
#include <cassert>


// A front is a set of mutually non-dominated points.
class ParetoFront
{
public:
	ParetoFront()
	: m_objectives(0)
	{ }

	bool empty() const
	{ return m_points.empty(); }
	std::size_t size() const
	{ return m_points.size(); }
	Vector const& operator [] (std::size_t index) const
	{ return m_points[index]; }
	std::vector<Vector> const& points() const
	{ return m_points; }

	// The reported number of objectives is zero after construction and
	// after clear(), before the first call to insert.
	std::size_t objectives() const
	{ return m_objectives; }

	// remove all points
	void clear();

	// update the non-dominated set, return true if the point was added
	bool insert(Vector const& point);

private:
	std::size_t m_objectives;
	std::vector<Vector> m_points;
};
