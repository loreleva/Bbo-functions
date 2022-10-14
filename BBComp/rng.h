
#pragma once

#include <vector>
#include "vector.h"
#include "matrix.h"


//
// Random Number Generator
//
class RNG
{
public:
	RNG(unsigned int inseed = 1);

	void seed(unsigned int inseed);

	int seed()
	{ return startseed; }

	long discrete(long min, long max);
	double uniform();
	double uniform(double min, double max);
	double gauss();

	Vector gaussVector(std::size_t dimension);
	Vector unitVector(std::size_t dimension);
	Matrix orthogonalMatrix(std::size_t dimension);

	long operator () (long num)
	{ return discrete(0, num - 1); }

private:
	// elementary randomness
	long random_long();

	// internal state
	int startseed;
	int aktseed;
	int aktrand;
	std::vector<int> rgrand;

	// Gaussians are created in pairs, store one
	bool flgstored;
	double hold;
};
