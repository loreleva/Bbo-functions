
#include "paretofront.h"

#include <set>
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <cstdlib>
#include <iostream>


using namespace std;


void ParetoFront::clear()
{
	m_points.clear();
	m_objectives = 0;
}

bool ParetoFront::insert(Vector const& point)
{
	if (m_objectives == 0)
	{
		assert(m_points.empty());
		m_objectives = point.size();
	}
	else assert(point.size() == m_objectives);

	for (size_t j=0; j<m_points.size(); j++)
	{
		Vector& x = m_points[j];
		int dom = 3;
		for (size_t i=0; i<m_objectives; i++)
		{
			if (point[i] < x[i]) dom &= 1;
			else if (x[i] < point[i]) dom &= 2;
			if (dom == 0) break;
		}
		if (dom & 2) return false;
		else if (dom == 1)
		{
			Vector& y = m_points.back();
			x = y;
			m_points.pop_back();
			j--;
		}
	}
	m_points.push_back(point);
	return true;
}
