
#include "matrix.h"
#ifdef _WIN32
#include <algorithm>	// otherwise "'min' : is not a member of 'std'"  
#endif

using namespace std;


Matrix operator * (double lhs, Matrix const& rhs)
{
	return (rhs * lhs);
}

Matrix Matrix::inverse() const
{
	size_t m = rows();
	size_t n = cols();

	// singular value decomposition
	Matrix U(*this);
	Matrix V(n, n);
	Vector D(n);

	size_t i, j, jj, k, l, nm(0);
	double anorm, c, f, g, h, s, scale, x, y, z;

	Vector rv1(n);

	g = scale = anorm = 0.0;

	for (i=0; i<n; i++)
	{
		l = i + 1;
		rv1(i) = scale * g;
		g = s = scale = 0.0;

		if (i < m)
		{
			for (k = i; k < m; k++) scale += U(k, i);

			if (scale != 0.0)
			{
				for (k = i; k < m; k++)
				{
					U(k, i) /= scale;
					s += U(k, i) * U(k, i);
				}

				f = U(i, i);
				g = (f > 0.0) ? -::sqrt(s) : ::sqrt(s);
				h = f * g - s;
				U(i, i) = f - g;

				for (j = l; j < n; j++)
				{
					s = 0.0;
					for (k = i; k < m; k++) s += U(k, i) * U(k, j);

					f = s / h;
					for (k = i; k < m; k++) U(k, j) += f * U(k, i);
				}

				for (k = i; k < m; k++) U(k, i) *= scale;
			}
		}

		D(i) = scale * g;
		g = s = scale = 0.0;

		if (i < m && i != n - 1)
		{
			for (k = l; k < n; k++) scale += U(i, k);

			if (scale != 0.0)
			{
				for (k = l; k < n; k++)
				{
					U(i, k) /= scale;
					s += U(i, k) * U(i, k);
				}

				f = U(i, l);
				g = (f > 0.0) ? -::sqrt(s) : ::sqrt(s);
				h = f * g - s;
				U(i, l) = f - g;

				for (k = l; k < n; k++) rv1(k) = U(i, k) / h;

				for (j = l; j < m; j++)
				{
					s = 0.0;
					for (k = l; k < n; k++) s += U(j, k) * U(i, k);
					for (k = l; k < n; k++) U(j, k) += s * rv1(k);
				}

				for (k = l; k < n; k++) U(i, k) *= scale;
			}
		}

		anorm = std::max(anorm, D(i) + rv1(i));
	}

	for (l = i = n; i--; l--)
	{
		if (l < n)
		{
			if (g != 0.0)
			{
				for (j = l; j < n; j++) V(j, i) = (U(i, j) / U(i, l)) / g;

				for (j = l; j < n; j++)
				{
					s = 0.0;
					for (k = l; k < n; k++) s += U(i, k) * V(k, j);
					for (k = l; k < n; k++) V(k, j) += s * V(k, i);
				}
			}

			for (j = l; j < n; j++) V(i, j) = V(j, i) = 0.0;
		}

		V(i, i) = 1.0;
		g = rv1(i);
	}

	for (l = i = std::min(m, n); i--; l--)
	{
		g = D(i);

		for (j = l; j < n; j++) U(i, j) = 0.0;

		if (g != 0.0)
		{
			g = 1.0 / g;
			for (j = l; j < n; j++)
			{
				s = 0.0;
				for (k = l; k < m; k++) s += U(k, i) * U(k, j);
				f = (s / U(i, i)) * g;
				for (k = i; k < m; k++) U(k, j) += f * U(k, i);
			}
			for (j = i; j < m; j++) U(j, i) *= g;
		}
		else
		{
			for (j = i; j < m; j++) U(j, i) = 0.0;
		}

		U(i, i)++;
	}

	for (k=n; k--;)
	{
		for (size_t its=1; ; its++)
		{
			bool flag = true;
			for (l = k; l > 0; l--)
			{
				nm = l - 1;
				if (rv1(l) + anorm == anorm)
				{
					flag = false;
					break;
				}
				if (D(nm) + anorm == anorm) break;
			}

			if (flag)
			{
				c = 0.0;
				s = 1.0;

				for (i = l; i <= k; i++)
				{
					f = s * rv1(i);
					rv1(i) *= c;

					if (f + anorm == anorm) break;

					g = D(i);
					h = hypot(f, g);
					D(i) = h;
					h = 1.0 / h;
					c = g * h;
					s = -f * h;

					for (j = 0; j < m; j++)
					{
						y = U(j, nm);
						z = U(j, i);
						U(j, nm) = y * c + z * s;
						U(j, i ) = z * c - y * s;
					}
				}
			}

			z = D(k);

			if (l == k)
			{
				if (z < 0.0)
				{
					D(k) = -z;
					for (j = 0; j < n; j++) V(j, k) = -V(j, k);
				}
				break;
			}

			if (its == 200) break;

			x = D(l);
			nm = k - 1;
			y = D(nm);
			g = rv1(nm);
			h = rv1(k);
			f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
			g = hypot(f, 1.0);
			f = ((x - z) * (x + z) + h * ((y / (f + ((f > 0.0) ? g : -g))) - h)) / x;

			c = s = 1.0;

			for (j = l; j < k; j++)
			{
				i = j + 1;
				g = rv1(i);
				y = D(i);
				h = s * g;
				g *= c;
				z = hypot(f, h);
				rv1(j) = z;
				c = f / z;
				s = h / z;
				f = x * c + g * s;
				g = g * c - x * s;
				h = y * s;
				y *= c;

				for (jj = 0; jj < n; jj++)
				{
					x = V(jj, j);
					z = V(jj, i);
					V(jj, j) = x * c + z * s;
					V(jj, i) = z * c - x * s;
				}

				z = hypot(f, h);
				D(j) = z;

				if (z != 0.0)
				{
					z = 1.0 / z;
					c = f * z;
					s = h * z;
				}

				f = c * g + s * y;
				x = c * y - s * g;

				for (jj = 0; jj < m; jj++)
				{
					y = U(jj, j);
					z = U(jj, i);
					U(jj, j) = y * c + z * s;
					U(jj, i) = z * c - y * s;
				}
			}

			rv1(l) = 0.0;
			rv1(k) = f;
			D(k) = x;
		}
	}

	// actual (generalized) inversion
	for (size_t i=0; i<n; i++)
	{
		D(i) = abs(D(i)) > 1e-10 ? 1.0 / D(i) : 0.0;
	}

	// compose final matrix
	Matrix ret(n, m);
	for (size_t i=0; i<n; i++)
	{
		for (size_t j=0; j<m; j++)
		{
			double v = 0.0;
			for (size_t k=0; k<n; k++) v += U(j, k) * D(k) * V(i, k);
			ret(i, j) = v;
		}
	}
	return ret;
}

Vector Matrix::eig(Matrix& U) const
{
	assert(m_rows == m_cols);
	assert(*this == transpose());

	U.resize(m_rows, m_cols);
	Vector lambda(m_rows);
	U = 0.0;
	lambda = 0.0;

	const size_t n = m_rows;
	Vector odvecA(n);

	size_t j, k, l, m;
	double b, c, f, g, h, hh, p, r, s, scale;

	if (n == 1)
	{
		U(0, 0) = 1;
		lambda(0) = (*this)(0, 0);
		return lambda;
	}

	U = *this;

	//
	// reduction to tridiagonal form
	//
	for (size_t i = n; i-- > 1;)
	{
		h = 0.0;
		scale = 0.0;

		if (i > 1)
		{
			// scale row
			for (size_t k = 0; k < i; k++) scale += abs(U(i, k));
		}

		if (scale == 0.0)
		{
			odvecA(i) = U(i, i-1);
		}
		else
		{
			for (k = 0; k < i; k++)
			{
				U(i, k) /= scale;
				h += U(i, k) * U(i, k);
			}

			f           = U(i, i-1);
			g           = f > 0 ? -std::sqrt(h) : std::sqrt(h);
			odvecA(i)   = scale * g;
			h          -= f * g;
			U(i, i-1) = f - g;
			f           = 0.0;

			for (j = 0; j < i; j++)
			{
				U(j, i) = U(i, j) / (scale * h);
				g = 0.0;

				// form element of a*u
				for (k = 0; k <= j; k++) g += U(j, k) * U(i, k);
				for (k = j + 1; k < i; k++) g += U(k, j) * U(i, k);

				// form element of p
				f += (odvecA(j) = g / h) * U(i, j);
			}

			hh = f / (h + h);

			// form reduced a
			for (j = 0; j < i; j++)
			{
				f     = U(i, j);
				g     = odvecA(j) - hh * f;
				odvecA(j) = g;

				for (k = 0; k <= j; k++) U(j, k) -= f * odvecA(k) + g * U(i, k);
			}

			for (k = i; k--;) U(i, k) *= scale;
		}

		lambda(i) = h;
	}

	lambda(0) = odvecA(0) = 0.0;

	// accumulation of transformation matrices
	for (size_t i = 0; i < n; i++)
	{
		if (lambda(i))
		{
			for (j = 0; j < i; j++)
			{
				g = 0.0;
				for (k = 0; k < i; k++) g += U(i, k) * U(k, j);
				for (k = 0; k < i; k++) U(k, j) -= g * U(k, i);
			}
		}

		lambda(i)     = U(i, i);
		U(i, i) = 1.0;

		for (j = 0; j < i; j++) {
			U(i, j) = U(j, i) = 0.0;
		}
	}

	//
	// eigen values from tridiagonal form
	//
	if (n <= 1) return lambda;

	for (size_t i = 1; i < n; i++) odvecA(i-1) = odvecA(i);

	odvecA(n-1) = 0.0;

	for (l = 0; l < n; l++)
	{
		j = 0;

		do
		{
			// look for small sub-diagonal element
			for (m = l; m < n - 1; m++)
			{
				s = std::fabs(lambda(m)) + std::fabs(lambda(m+1));
				if (std::fabs(odvecA(m)) + s == s) break;
			}

			p = lambda(l);

			if (m != l)
			{
				if (j++ == 200)
				{
					// Too many iterations --> numerical instability!
					break;
				}

				// form shift
				g = (lambda(l+1) - p) / (2.0 * odvecA(l));
				r = std::sqrt(g * g + 1.0);
				g = lambda(m) - p + odvecA(l) / (g + ((g) > 0 ? std::fabs(r) : -std::fabs(r)));
				s = c = 1.0;
				p = 0.0;

				for (size_t i = m; i-- > l;)
				{
					f = s * odvecA(i);
					b = c * odvecA(i);

					if (std::fabs(f) >= std::fabs(g))
					{
						c       = g / f;
						r       = std::sqrt(c * c + 1.0);
						odvecA(i+1) = f * r;
						s       = 1.0 / r;
						c      *= s;
					}
					else
					{
						s       = f / g;
						r       = std::sqrt(s * s + 1.0);
						odvecA(i+1) = g * r;
						c       = 1.0 / r;
						s      *= c;
					}

					g       = lambda(i+1) - p;
					r       = (lambda(i) - g) * s + 2.0 * c * b;
					p       = s * r;
					lambda(i+1) = g + p;
					g       = c * r - b;

					// form vector
					for (k = 0; k < n; k++)
					{
						f           = U(k, i+1);
						U(k, i+1) = s * U(k, i) + c * f;
						U(k, i  ) = c * U(k, i) - s * f;
					}
				}

				lambda(l) -= p;
				odvecA(l)  = g;
				odvecA(m)  = 0.0;
			}
		}
		while (m != l);
	}

	// normalize eigen vectors
	for (size_t j=0; j<n; j++)
	{
		s = 0.0;
		for (size_t i=0; i<n; i++)
		{
			const double v = U(i, j);
			s += v * v;
		}
		s = std::sqrt(s);
		for (size_t i=0; i<n; i++) U(i, j) /= s;
	}

	// return eigen values
	return lambda;
}

std::ostream& operator << (std::ostream& str, Matrix const& mat)
{
	str << "[";
	if (! mat.empty())
	{
		for (size_t r=0; r<mat.rows(); r++)
		{
			if (r != 0) str << "; ";
			str << mat(r, 0);
			for (size_t c=1; c<mat.cols(); c++) str << ", " << mat(r, c);
		}
	}
	str << "]";
	return str;
}
