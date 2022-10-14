
#include "hypervolume.h"

#include <map>
#include <algorithm>
#include <stdexcept>
#include <cassert>


using namespace std;


////////////////////////////////////////////////////////////
// comparator for sorting by last objective
// (lexicographically, reading from right to left)
//
struct ComparatorIndicesLast
{
	ComparatorIndicesLast(ParetoFront const& front)
	: m_front(front)
	{ }

	ParetoFront const& m_front;

	bool operator () (size_t i, size_t j)
	{
		Vector const& vi = m_front[i];
		Vector const& vj = m_front[j];
		assert(vi.size() == vj.size());
		for (int index = vi.size() - 1; index >= 0; index--)
		{
			if (vi[index] < vj[index]) return true;
			if (vi[index] > vj[index]) return false;
		}
		return false;
	}
};


////////////////////////////////////////////////////////////
// 2D and 3D hypervolume computation in N log(N) operations.
// The algorithm assumes that points don't dominate each other,
// and that they all dominate the reference point.
//
double hypervolume(Vector const& reference, ParetoFront const& front)
{
	size_t N = front.size();
	if (N == 0) return 0.0;

	// sort index array by last objective
	ComparatorIndicesLast comparator_last(front);
	vector<size_t> order(N);
	for (size_t i=0; i<N; i++) order[i] = i;
	sort(order.begin(), order.end(), comparator_last);

	if (reference.size() == 2)
	{
		// Instead of recomputing the hypervolume from scratch each
		// time we could update it rather cheaply by computing the
		// contribution of the new point. However, this won't safe us
		// in the 3D case, at least with the current algorithm.
		// Therefore I see no point in changing the interfaces
		// accordingly.
		double area = (reference[0] - front[order[0]][0]) * (reference[1] - front[order[0]][1]);
		for (size_t i=1; i<front.size(); i++)
		{
			area += (front[order[i-1]][0] - front[order[i]][0]) * (reference[1] - front[order[i]][1]);
		}
		return area;
	}
	else if (reference.size() == 3)
	{
		// 10000 hypervolume computations of 1...10000 points take
		// about 18 seconds on my laptop. This is an upper bound for
		// what we need for a problem with a budget of 10000 function
		// evaluations. For 1000 such problems this amounts to 18000
		// seconds (5 hours) for the full competition, which is okay
		// as an upper bound. Therefore iterative computation of the
		// hypervolume is not a top priority right now.
		double volume = 0.0;
		double area = 0.0;
		map<double, double> front2D;
		double prev_z = 0.0;
		for (size_t i=0; i<N; i++)
		{
			Vector const& x = front[order[i]];
			if (i > 0) volume += area * (x[2] - prev_z);

			// check whether x is dominated
			map<double, double>::iterator worse = front2D.upper_bound(x[0]);
			double b = reference[1];
			if (worse != front2D.begin())
			{
				map<double, double>::iterator better = worse;
				if (better == front2D.end() || better->first > x[0]) --better;
				if (better->second <= x[1]) continue;
				b = better->second;
			}

			// remove dominated points
			while (worse != front2D.end())
			{
				if (worse->second < x[1]) break;
				map<double, double>::iterator it = worse;
				++worse;
				double r = (worse == front2D.end()) ? reference[0]: worse->first;
				area -= (r - it->first) * (b - it->second);
				front2D.erase(it);
			}

			// insert x
			front2D[x[0]] = x[1];
			double r = (worse == front2D.end()) ? reference[0] : worse->first;
			area += (r - x[0]) * (b - x[1]);

			prev_z = x[2];
		}
		volume += area * (reference[2] - prev_z);
		return volume;
	}
	else throw runtime_error("[hypervolume] number of objectives must be 2 or 3");
}
