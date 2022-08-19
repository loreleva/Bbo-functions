
#pragma once


//
// Expression Interpreter
// ----------------------
//
// The expression interpreter takes a numerical expression as a string
// and turns it into an evaluable C++ object. This object essentially
// acts as an interpreter for the computation represented by the string.
// Elementary operations are represented by sub-objects with a virtual
// eval(...) function, hence, essentially by function pointers. This
// approach has minimal runtime overhead over compiled C++ expressions.
//
// There are two data types, scalars and vectors.
// Expressions are formed as follows:
//
// (*) Numerical constants like "-7" and "3.1415926536" are scalars.
// (*) Variable "x", vector argument of the expression
// (*) Variable "lambda", scalar argument of a lambda function (see apply)
// (*) Left unary negation operator "-".
// (*) Binary operators "+", "-", "*", "/", and "^" (power).
//     The operators are applicable to scalars and vectors as usual.
//     The product of two vectors is defined as the (standard) inner
//     product.
// (*) Binary element-wise operators ".*", "./", and ".^".
// (*) Extraction of scalar component from vector, e.g., "x[2]";
//     indices start from 1.
// (*) Extraction of vector range from vector, e.g., "x[2:5]";
//     indices start from 1, both bounds included.
// (*) Round brackets around arbitrary expressions.
// (*) Vectors formed from scalars and sub-vectors, e.g., "[2,-3]" or
//     "[x, 1]" for a vector containing all components of "x" and an
//     additional component with value 1.
// (*) Elementary functions transforming scalars: sqr (square), sqrt,
//     exp, log, sin, cos, tan, sinh, cosh, tanh, asin, acos, atan,
//     abs (absolute value), floor (round down), ceil (round up),
//     round (round to closest integer).
// (*) Vector expressions "zeros(N)", "ones(N)", "range(N)" yield
//     vectors [0, ..., 0], [1, ..., 1], and [1, 2, 3, ..., N],
//     respectively.
// (*) Scalar properties of vectors: "sum(x)" and "prod(x)" compute the
//     sum and the product of vector entries, "dim(x)" returns the
//     number of entries, "norm(x)" computes the Euclidean norm of x,
//     "sqrnorm(x)" computes its square, "min(x)" computes the minimum
//     over the entries, and "max(x)" computes the maximum over the
//     entries of the vector.
// (*) "apply(x, f)" returns the result of applying function f to each
//     component of vector x. Function f itself is an expression where
//     the symbol "lambda" represents the scalar component of x.
// (*) The expression may start with arbitrarily many variable
//     definitions of the form "var <name> = <expression> ;" where
//     <name> is an identifier. Variable definitions are useful, e.g.,
//     for transformations of the input.
// (*) An identifier represents the value of a variable, which must have
//     been defined before it is used.
//
// This is a purely functional style programming language for the
// definition of expressions. If avoids explicit loops in the
// interpreter.
//
// Example 1: The ellipsoid function
//		var d = dim(x);
//		var y = x - 0.5 * ones(d);
//		(apply(range(d), 1e6 ^ ((lambda-1) / (d-1)))) * (y .* y)
//
// The variable "d" is defined to hold the problem dimension. The
// variable "y" shifts the optimum to (0.5, ..., 0.5), which is the
// center of the feasible region (unit hypercube). The actual expression
// applies the function "f(lambda) = 1e6 ^ ((lambda-1) / (d-1))" to each
// component of the vector "range(d)", resulting in the component-wise
// factors of the ellipsoid function. These components are then
// multiplied with the components of the sphere function, found in the
// vector "y .* y".
//
// Example 2: Rosenbrock's function
//		var d = dim(x);
//		var xm = x[1:d-1];
//		sqrnorm(ones(d-1) - xm) + 100 * sqrnorm(x[2:d] - xm .* xm)
//


#include <string>
#include <memory>

#include "vector.h"


// expression types
struct Expression;
typedef std::shared_ptr<Expression> ExpressionPtr;


// actual interface
ExpressionPtr parse(std::string str);
double evaluate(ExpressionPtr ex, Vector const& x);
