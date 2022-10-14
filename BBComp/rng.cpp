
#include "rng.h"

#include <stdexcept>
#include <cmath>


#ifdef _MSC_VER
typedef __int64 my_int64_t;
#elif __INTEL_COMPILER
typedef __int64 my_int64_t;
#else
typedef int64_t my_int64_t;
#endif


RNG::RNG(unsigned int inseed)
: rgrand(32)
, flgstored(false)
{ seed(inseed); }


void RNG::seed(unsigned int inseed)
{
	flgstored = false;
	startseed = inseed;
	if (inseed < 1) inseed = 1; 
	aktseed = inseed;
	for (int i = 39; i >= 0; --i)
	{
		long tmp = aktseed / 127773;
		aktseed = 16807 * (aktseed - tmp * 127773) - 2836 * tmp;
		if (aktseed < 0) aktseed += 2147483647;
		if (i < 32) rgrand[i] = aktseed;
	}
	aktrand = rgrand[0];
}

long RNG::discrete(long min, long max)
{
	if (min > max) throw std::runtime_error("[RNG::discrete] invalid parameters");
	my_int64_t diff = (my_int64_t)max - (my_int64_t)min;
	if (diff >= 0x80000000LL) throw std::runtime_error("[RNG::discrete] invalid parameters");
	return (min + random_long() % (max + 1 - min));
}

double RNG::uniform()
{
	return (double)(random_long()) / (2.147483647e9);
}

double RNG::uniform(double min, double max)
{
	return min + (max - min) * uniform();
}

double RNG::gauss()
{
	if (flgstored)
	{    
		flgstored = false;
		return hold;
	}

	double x1, x2, rquad, fac;
	do
	{
		x1 = 2.0 * uniform() - 1.0;
		x2 = 2.0 * uniform() - 1.0;
		rquad = x1*x1 + x2*x2;
	}
	while (rquad >= 1.0 || rquad <= 0.0);
	fac = sqrt(-2.0 * log(rquad) / rquad);
	flgstored = true;
	hold = fac * x1;
	return fac * x2;
}

Vector RNG::gaussVector(std::size_t dimension)
{
	Vector ret(dimension);
	for (std::size_t i=0; i<dimension; i++) ret[i] = gauss();
	return ret;
}

Vector RNG::unitVector(std::size_t dimension)
{
	if (dimension == 0) throw std::runtime_error("[RNG::unitVector] dimension must be positive");
	Vector ret = gaussVector(dimension);
	return ret / ret.twonorm();
}

Matrix RNG::orthogonalMatrix(std::size_t dimension)
{
	if (dimension == 0) throw std::runtime_error("[RNG::orthogonalMatrix] dimension must be positive");

	// Gram-Schmidt orthogonalization of a random basis
	Matrix ret(dimension, dimension);
	for (std::size_t i=0; i<dimension; i++)
	{
		Vector v = gaussVector(dimension);
		for (std::size_t j=0; j<i; j++)
		{
			double s = 0.0;
			for (std::size_t k=0; k<dimension; k++) s += ret(j, k) * v(k);
			for (std::size_t k=0; k<dimension; k++) v(k) -= s * ret(j, k);
		}
		v /= v.twonorm();
		for (std::size_t k=0; k<dimension; k++) ret(i, k) = v(k);
	}
	return ret;
}

long RNG::random_long()
{
	int tmp = aktseed / 127773;
	aktseed = 16807 * (aktseed - tmp * 127773) - 2836 * tmp;
	if (aktseed < 0) aktseed += 2147483647;
	tmp = aktrand / 67108865;
	aktrand = rgrand[tmp];
	rgrand[tmp] = aktseed;
	return aktrand;
}
