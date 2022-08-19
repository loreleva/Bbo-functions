
#include "interpreter.h"
#include "parser.h"

#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cctype>
#include <cassert>
#include <vector>
#include <algorithm>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif


using namespace std;


////////////////////////////////////////////////////////////


struct ExpressionBase
{
	virtual ~ExpressionBase()
	{ }
};
typedef shared_ptr< ExpressionBase > BaseExPtr;

template <typename VAR>
struct ExpressionU : public ExpressionBase
{
};

template <typename VAR>
struct ParameterTraits
{
	static const bool isScalar = false;
	static const bool isVector = false;
};
template <>
struct ParameterTraits<double>
{
	static const bool isScalar = true;
	static const bool isVector = false;
};
template <>
struct ParameterTraits<Vector>
{
	static const bool isScalar = false;
	static const bool isVector = true;
};

template <typename VAR>
struct ExPtr {
    typedef shared_ptr< ExpressionU<VAR> > type;
};

template <typename T, typename VAR>
struct ExpressionT : public ExpressionU<VAR>
{
	virtual T eval(VAR const& x) const = 0;
};

template <typename T, typename VAR>
struct ExPtrT {
    typedef shared_ptr< ExpressionT<T, VAR> > type;
};

typedef ExpressionT<double, Vector> SVExpression;
typedef ExPtrT<double, Vector>::type SVExPtrT;
typedef ExpressionT<Vector, Vector> VVExpression;
typedef ExPtrT<Vector, Vector>::type VVExPtrT;

template <typename VAR>
bool isScalar(typename ExPtr<VAR>::type p) { return (dynamic_pointer_cast< ExpressionT<double, VAR> >(p) != NULL); }
template <typename VAR>
bool isVector(typename ExPtr<VAR>::type p) { return (dynamic_pointer_cast< ExpressionT<Vector, VAR> >(p) != NULL); }
template <typename VAR>
typename ExPtrT<double, VAR>::type asScalar(typename ExPtr<VAR>::type p) { return (static_pointer_cast< ExpressionT<double, VAR> >(p)); }
template <typename VAR>
typename ExPtrT<Vector, VAR>::type asVector(typename ExPtr<VAR>::type p) { return (static_pointer_cast< ExpressionT<Vector, VAR> >(p)); }

template <typename T, typename VAR>
struct Constant : public ExpressionT<T, VAR>
{
	Constant(T v)
	: value(v)
	{ }

	T eval(VAR const& x) const
	{ return value; }

	T value;
};
typedef Constant<double, double> SSConstant;
typedef Constant<Vector, double> VSConstant;
typedef Constant<double, Vector> SVConstant;
typedef Constant<Vector, Vector> VVConstant;

template <typename VAR>
bool isConstant(typename ExPtr<VAR>::type p) { return isScalar<VAR>(p) ? (dynamic_pointer_cast< Constant<double, VAR> >(p) != NULL) : (dynamic_pointer_cast< Constant<Vector, VAR> >(p) != NULL); }


template <typename T>
struct Variable : public ExpressionT<T, T>
{
	Variable()
	{ }

	T eval(T const& x) const
	{ return x; }
};

struct AuxiliaryVariableBase
{
	AuxiliaryVariableBase(string name_)
	: name(name_)
	{ }

	virtual ~AuxiliaryVariableBase()
	{ }

	virtual void preeval(Vector const& x) = 0;

	string name;
};

template <typename T>
struct AuxiliaryVariable : public AuxiliaryVariableBase
{
	AuxiliaryVariable(string name_, typename ExPtrT<T, Vector>::type ex_)
	: AuxiliaryVariableBase(name_)
	, ex(ex_)
	{ }

	void preeval(Vector const& x)
	{ value = ex->eval(x); }

	T eval() const
	{ return value; }

	typename ExPtrT<T, Vector>::type ex;
	T value;
};
typedef AuxiliaryVariable<double> SAuxiliaryVariable;
typedef AuxiliaryVariable<Vector> VAuxiliaryVariable;

bool isScalar(shared_ptr<AuxiliaryVariableBase> p) { return (dynamic_pointer_cast<SAuxiliaryVariable>(p) != NULL); }
bool isVector(shared_ptr<AuxiliaryVariableBase> p) { return (dynamic_pointer_cast<VAuxiliaryVariable>(p) != NULL); }
shared_ptr<SAuxiliaryVariable> asScalar(shared_ptr<AuxiliaryVariableBase> p) { return (static_pointer_cast<SAuxiliaryVariable>(p)); }
shared_ptr<VAuxiliaryVariable> asVector(shared_ptr<AuxiliaryVariableBase> p) { return (static_pointer_cast<VAuxiliaryVariable>(p)); }

template <typename T, typename VAR>
struct VariableReference : public ExpressionT<T, VAR>
{
	VariableReference(shared_ptr< AuxiliaryVariable<T> > aux_)
	: aux(aux_)
	{ }

	T eval(VAR const& x) const
	{ return aux->eval(); }

	shared_ptr< AuxiliaryVariable<T> > aux;
};

template <typename T, typename VAR>
struct Negation : public ExpressionT<T, VAR>
{
	Negation(typename ExPtrT<T, VAR>::type ex_)
	: ex(ex_)
	{ }

	T eval(VAR const& x) const
	{ return -ex->eval(x); }

	typename ExPtrT<T, VAR>::type ex;
};
typedef Negation<double, double> SSNegation;
typedef Negation<Vector, double> VSNegation;
typedef Negation<double, Vector> SVNegation;
typedef Negation<Vector, Vector> VVNegation;

template <typename LHS, typename RHS, typename RET, typename VAR>
struct BinaryOperator : public ExpressionT<RET, VAR>
{
	BinaryOperator(typename ExPtrT<LHS, VAR>::type lhs_, typename ExPtrT<RHS, VAR>::type rhs_)
	: lhs(lhs_)
	, rhs(rhs_)
	{ }

	typename ExPtrT<LHS, VAR>::type lhs;
	typename ExPtrT<RHS, VAR>::type rhs;
};

template <typename T, typename VAR>
struct Sum : public BinaryOperator<T, T, T, VAR>
{
	typedef BinaryOperator<T, T, T, VAR> BaseType;

	Sum(typename ExPtrT<T, VAR>::type l, typename ExPtrT<T, VAR>::type r)
	: BaseType(l, r)
	{ }

	T eval(VAR const& x) const
	{ return BaseType::lhs->eval(x) + BaseType::rhs->eval(x); }
};

template <typename T, typename VAR>
struct Difference : public BinaryOperator<T, T, T, VAR>
{
	typedef BinaryOperator<T, T, T, VAR> BaseType;

	Difference(typename ExPtrT<T, VAR>::type l, typename ExPtrT<T, VAR>::type r)
	: BaseType(l, r)
	{ }

	T eval(VAR const& x) const
	{ return BaseType::lhs->eval(x) - BaseType::rhs->eval(x); }
};

template <typename LHS, typename RHS, typename RET, typename VAR>
struct Product : public BinaryOperator<LHS, RHS, RET, VAR>
{
	typedef BinaryOperator<LHS, RHS, RET, VAR> BaseType;

	Product(typename ExPtrT<LHS, VAR>::type l, typename ExPtrT<RHS, VAR>::type r)
	: BaseType(l, r)
	{ throw runtime_error("invalid specialization"); }

	RET eval(VAR const& x) const
	{ throw runtime_error("invalid specialization"); }
};

template <typename VAR>
struct Product<Vector, Vector, double, VAR> : public BinaryOperator<Vector, Vector, double, VAR>
{
	typedef BinaryOperator<Vector, Vector, double, VAR> BaseType;

	Product(typename ExPtrT<Vector, VAR>::type l, typename ExPtrT<Vector, VAR>::type r)
	: BaseType(l, r)
	{ }

	double eval(VAR const& x) const
	{ return BaseType::lhs->eval(x) * BaseType::rhs->eval(x); }
};

template <typename VAR>
struct Product<Vector, double, Vector, VAR> : public BinaryOperator<Vector, double, Vector, VAR>
{
	typedef BinaryOperator<Vector, double, Vector, VAR> BaseType;

	Product(typename ExPtrT<Vector, VAR>::type l, typename ExPtrT<double, VAR>::type r)
	: BaseType(l, r)
	{ }

	Vector eval(VAR const& x) const
	{ return BaseType::lhs->eval(x) * BaseType::rhs->eval(x); }
};

template <typename VAR>
struct Product<double, Vector, Vector, VAR> : public BinaryOperator<double, Vector, Vector, VAR>
{
	typedef BinaryOperator<double, Vector, Vector, VAR> BaseType;

	Product(typename ExPtrT<double, VAR>::type l, typename ExPtrT<Vector, VAR>::type r)
	: BaseType(l, r)
	{ }

	Vector eval(VAR const& x) const
	{ return BaseType::lhs->eval(x) * BaseType::rhs->eval(x); }
};

template <typename VAR>
struct Product<double, double, double, VAR> : public BinaryOperator<double, double, double, VAR>
{
	typedef BinaryOperator<double, double, double, VAR> BaseType;

	Product(typename ExPtrT<double, VAR>::type l, typename ExPtrT<double, VAR>::type r)
	: BaseType(l, r)
	{ }

	double eval(VAR const& x) const
	{ return BaseType::lhs->eval(x) * BaseType::rhs->eval(x); }
};

template <typename T, typename VAR>
struct Quotient : public BinaryOperator<T, double, T, VAR>
{
	typedef BinaryOperator<T, double, T, VAR> BaseType;

	Quotient(typename ExPtrT<T, VAR>::type l, typename ExPtrT<double, VAR>::type r)
	: BaseType(l, r)
	{ }

	T eval(VAR const& x) const
	{ return BaseType::lhs->eval(x) / BaseType::rhs->eval(x); }
};

template <typename VAR>
struct Power : public BinaryOperator<double, double, double, VAR>
{
	typedef BinaryOperator<double, double, double, VAR> BaseType;

	Power(typename ExPtrT<double, VAR>::type l, typename ExPtrT<double, VAR>::type r)
	: BaseType(l, r)
	{ }

	double eval(VAR const& x) const
	{ return pow(BaseType::lhs->eval(x), BaseType::rhs->eval(x)); }
};

template <typename VAR>
struct ElemProduct : public BinaryOperator<Vector, Vector, Vector, VAR>
{
	typedef BinaryOperator<Vector, Vector, Vector, VAR> BaseType;

	ElemProduct(typename ExPtrT<Vector, VAR>::type l, typename ExPtrT<Vector, VAR>::type r)
	: BaseType(l, r)
	{ }

	Vector eval(VAR const& x) const
	{
		return BaseType::lhs->eval(x).elemProduct(BaseType::rhs->eval(x));
	}
};

template <typename VAR>
struct ElemQuotient : public BinaryOperator<Vector, Vector, Vector, VAR>
{
	typedef BinaryOperator<Vector, Vector, Vector, VAR> BaseType;

	ElemQuotient(typename ExPtrT<Vector, VAR>::type l, typename ExPtrT<Vector, VAR>::type r)
	: BaseType(l, r)
	{ }

	Vector eval(VAR const& x) const
	{
		return BaseType::lhs->eval(x).elemQuotient(BaseType::rhs->eval(x));
	}
};

template <typename RHS, typename VAR>
struct ElemPower : public BinaryOperator<Vector, RHS, Vector, VAR>
{
	typedef BinaryOperator<Vector, RHS, Vector, VAR> BaseType;

	ElemPower(typename ExPtrT<Vector, VAR>::type l, typename ExPtrT<RHS, VAR>::type r)
	: BaseType(l, r)
	{ }

	Vector eval(VAR const& x) const
	{
		return BaseType::lhs->eval(x).elemPower(BaseType::rhs->eval(x));
	}
};

template <typename VAR>
struct Dimension : public ExpressionT<double, VAR>
{
	Dimension(typename ExPtrT<Vector, VAR>::type arg_)
	: arg(arg_)
	{ }

	double eval(VAR const& x) const
	{ return (double)arg->eval(x).size(); }

	typename ExPtrT<Vector, VAR>::type arg;
};

template <typename VAR>
struct VectorComposition : public ExpressionT<Vector, VAR>
{
	void add(typename ExPtr<VAR>::type ex)
	{
		sub.push_back(ex);
		isV.push_back(isVector<VAR>(ex));
	}

	Vector eval(VAR const& x) const
	{
		vector<Vector> subvec(sub.size());
		size_t total = 0;
		for (size_t i=0; i<sub.size(); i++)
		{
			if (isV[i])
			{
				typename ExPtrT<Vector, VAR>::type ex = static_pointer_cast< ExpressionT<Vector, VAR> >(sub[i]);
				subvec[i] = ex->eval(x);
				total += subvec[i].size();
			}
			else total++;
		}
		Vector ret(total);
		size_t pos = 0;
		for (size_t i=0; i<sub.size(); i++)
		{
			if (isV[i])
			{
				Vector const& v = subvec[i];
				for (size_t j=0; j<v.size(); j++, pos++) ret[pos] = v[j];
			}
			else
			{
				typename ExPtrT<double, VAR>::type ex = static_pointer_cast< ExpressionT<double, VAR> >(sub[i]);
				double v = ex->eval(x);
				ret[pos] = v;
				pos++;
			}
		}
		return ret;
	}

	vector< typename ExPtr<VAR>::type > sub;
	vector< bool > isV;
};

template <typename VAR>
struct VectorEntry : public ExpressionT<double, VAR>
{
	VectorEntry(
			typename ExPtrT<Vector, VAR>::type base_,
			typename ExPtrT<double, VAR>::type index_)
	: base(base_)
	, index(index_)
	{ }

	double eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		int i = (int)floor(index->eval(x));
		if (i < 1 || i > (int)tmp.size()) throw runtime_error("index out of bounds");
		i--;
		return tmp[i];
	}

	typename ExPtrT<Vector, VAR>::type base;
	typename ExPtrT<double, VAR>::type index;
};

template <typename VAR>
struct VectorRange : public ExpressionT<Vector, VAR>
{
	VectorRange(
			typename ExPtrT<Vector, VAR>::type base_,
			typename ExPtrT<double, VAR>::type first_,
			typename ExPtrT<double, VAR>::type last_)
	: base(base_)
	, first(first_)
	, last(last_)
	{ }

	Vector eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		int f = (int)floor(first->eval(x));
		int l = (int)floor(last->eval(x));
		int size = l - f + 1;
		if (f < 1 || l > (int)tmp.size() || size < 0) throw runtime_error("dimension mismatch");
		int b = f - 1;
		int e = l;
		Vector ret((size_t)size);
		for (int i=b; i<e; i++) ret[i - b] = tmp[i];
		return ret;
	}

	typename ExPtrT<Vector, VAR>::type base;
	typename ExPtrT<double, VAR>::type first;
	typename ExPtrT<double, VAR>::type last;
};

template <typename VAR>
struct ConstVect : public ExpressionT<Vector, VAR>
{
	ConstVect(
			typename ExPtrT<double, VAR>::type size_,
			double value_)
	: size(size_)
	, value(value_)
	{ }

	Vector eval(VAR const& x) const
	{
		int sz = (int)floor(size->eval(x));
		if (sz < 0) throw runtime_error("dimension must be non-negative");
		return Vector(sz, value);
	}

	typename ExPtrT<double, VAR>::type size;
	double value;
};

template <typename VAR>
struct RangeVect : public ExpressionT<Vector, VAR>
{
	RangeVect(typename ExPtrT<double, VAR>::type size_)
	: size(size_)
	{ }

	Vector eval(VAR const& x) const
	{
		int sz = (int)floor(size->eval(x));
		if (sz < 0) throw runtime_error("dimension must be non-negative");
		Vector ret((size_t)sz);
		for (int i=0; i<sz; i++) ret[i] = i + 1;
		return ret;
	}

	typename ExPtrT<double, VAR>::type size;
};

template <typename VAR>
struct ComponentWiseOperation : public ExpressionT<Vector, VAR>
{
	ComponentWiseOperation(
			typename ExPtrT<Vector, VAR>::type base_,
			ExPtrT<double, double>::type func_)
	: base(base_)
	, func(func_)
	{ }

	Vector eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		for (size_t i=0; i<tmp.size(); i++)
		{
			tmp[i] = func->eval(tmp[i]);
		}
		return tmp;
	}

	typename ExPtrT<Vector, VAR>::type base;
	ExPtrT<double, double>::type func;
};

template <typename VAR>
struct SumAggregation : public ExpressionT<double, VAR>
{
	SumAggregation(typename ExPtrT<Vector, VAR>::type base_)
	: base(base_)
	{ }

	double eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		double ret = 0.0;
		for (size_t i=0; i<tmp.size(); i++) ret += tmp[i];
		return ret;
	}

	typename ExPtrT<Vector, VAR>::type base;
};

template <typename VAR>
struct ProductAggregation : public ExpressionT<double, VAR>
{
	ProductAggregation(typename ExPtrT<Vector, VAR>::type base_)
	: base(base_)
	{ }

	double eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		double ret = 1.0;
		for (size_t i=0; i<tmp.size(); i++) ret *= tmp[i];
		return ret;
	}

	typename ExPtrT<Vector, VAR>::type base;
};

template <typename VAR>
struct TwoNorm : public ExpressionT<double, VAR>
{
	TwoNorm(typename ExPtrT<Vector, VAR>::type base_)
	: base(base_)
	{ }

	double eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		double norm2 = 0.0;
		for (size_t i=0; i<tmp.size(); i++) norm2 += tmp[i] * tmp[i];
		return std::sqrt(norm2);
	}

	typename ExPtrT<Vector, VAR>::type base;
};

template <typename VAR>
struct TwoNorm2 : public ExpressionT<double, VAR>
{
	TwoNorm2(typename ExPtrT<Vector, VAR>::type base_)
	: base(base_)
	{ }

	double eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		double norm2 = 0.0;
		for (size_t i=0; i<tmp.size(); i++) norm2 += tmp[i] * tmp[i];
		return norm2;
	}

	typename ExPtrT<Vector, VAR>::type base;
};

template <typename VAR>
struct MinAggregation : public ExpressionT<double, VAR>
{
	MinAggregation(typename ExPtrT<Vector, VAR>::type base_)
	: base(base_)
	{ }

	double eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		double ret = tmp[0];
		for (size_t i=1; i<tmp.size(); i++) ret = std::min(ret, tmp[i]);
		return ret;
	}

	typename ExPtrT<Vector, VAR>::type base;
};

template <typename VAR>
struct MaxAggregation : public ExpressionT<double, VAR>
{
	MaxAggregation(typename ExPtrT<Vector, VAR>::type base_)
	: base(base_)
	{ }

	double eval(VAR const& x) const
	{
		Vector tmp = base->eval(x);
		double ret = tmp[0];
		for (size_t i=1; i<tmp.size(); i++) ret = std::max(ret, tmp[i]);
		return ret;
	}

	typename ExPtrT<Vector, VAR>::type base;
};

typedef double(*func)(double);

double round(double t)
{ return floor(t + 0.5); }

double sqr(double t)
{ return (t * t); }

template <typename VAR>
struct Function : public ExpressionT<double, VAR>
{
	Function(func f_, typename ExPtrT<double, VAR>::type inner_)
	: f(f_)
	, inner(inner_)
	{ }

	double eval(VAR const& x) const
	{ return f(inner->eval(x)); }

	func f;
	typename ExPtrT<double, VAR>::type inner;
};
typedef Function<double> SFunction;
typedef Function<Vector> VFunction;


////////////////////////////////////////////////////////////
// parser for simple expression language
//
class ExpressionParser
{
public:
	ExpressionParser()
	: scanner("#")
	, ex_bracket("bracket expression")
	, ex_vector("vector composition")
	, ex_access("vector entry")
	, ex_range("vector range")
	, ex_function("built-in function")
	, ex_apply("component-wise operation")
	, ex_simple("simple expression")
	, ex_negate("negation")
	, expression("expression")
	, aux("auxiliary variable definition")
	, root("root")
	{
		scanner.addToken(".*");
		scanner.addToken("./");
		scanner.addToken(".^");

		scanner.addKeyword("var");
		scanner.addKeyword("apply");
		scanner.addKeyword("dim");
		scanner.addKeyword("zeros");
		scanner.addKeyword("ones");
		scanner.addKeyword("range");
		scanner.addKeyword("abs");
		scanner.addKeyword("floor");
		scanner.addKeyword("ceil");
		scanner.addKeyword("round");
		scanner.addKeyword("sqr");
		scanner.addKeyword("sqrt");
		scanner.addKeyword("exp");
		scanner.addKeyword("log");
		scanner.addKeyword("log10");
		scanner.addKeyword("sin");
		scanner.addKeyword("cos");
		scanner.addKeyword("tan");
		scanner.addKeyword("sinh");
		scanner.addKeyword("cosh");
		scanner.addKeyword("tanh");
		scanner.addKeyword("asin");
		scanner.addKeyword("acos");
		scanner.addKeyword("atan");
		scanner.addKeyword("sum");
		scanner.addKeyword("prod");
		scanner.addKeyword("norm");
		scanner.addKeyword("sqrnorm");
		scanner.addKeyword("min");
		scanner.addKeyword("max");

		// symbols
		left_unary_operator_symbol = symboltable("left unary operator symbol", "-");
		binary_operator_symbol = symboltable("binary operator symbol", ".* ./ .^ ^ * / % + -");

		// expressions
		ex_bracket = "(" >> applies >> expression >> ")";
		ex_vector = "[" >> applies >> (expression % ",") >> "]";
		ex_access = "[" >> expression >> "]";
		ex_range = "[" >> expression >> ":" >> applies >> expression >> "]";
		ex_function = (key("dim") | key("zeros") | key("ones") | key("range")
					| key("abs")
					| key("floor") | key("ceil") | key("round")
					| key("sqr") | key("sqrt") | key("exp") | key("log") | key("log10")
					| key("sin") | key("cos") | key("tan")
					| key("sinh") | key("cosh") | key("tanh")
					| key("asin") | key("acos") | key("atan")
					| key("sum") | key("prod") | key("norm") | key("sqrnorm") | key("min") | key("max"))
				>> applies >> "(" >> expression >> ")";
		ex_apply = key("apply") >> applies >> "(" >> expression >> "," >> expression >> ")";
		ex_simple = (ex_bracket | ex_vector | ex_function | ex_apply | floatingpoint | identifier) >> *ex_range >> -ex_access;
		ex_negate = left_unary_operator_symbol >> applies >> ex_simple;
		expression = (ex_negate | ex_simple) / binary_operator_symbol;

		// auxiliary variable definition
		aux = key("var") >> applies >> identifier >> "=" >> expression >> ";";

		// start (root) rule
		root = *aux >> expression;
	};

	Node* parse(string expression)
	{
		stringstream stream;
		if (! scanner.scan(expression, stream)) throw runtime_error(stream.str());
		string error;
		Node* ret = root.parseAll(scanner, error);
		if (ret == NULL) throw runtime_error(error);
		return ret;
	}

protected:
	DefaultScanner scanner;

	Parser left_unary_operator_symbol;
	Parser binary_operator_symbol;

	Rule ex_bracket;
	Rule ex_vector;
	Rule ex_access;
	Rule ex_range;
	Rule ex_function;
	Rule ex_apply;

	Rule ex_simple;
	Rule ex_negate;
	Rule expression;

	Rule aux;

	Rule root;
};


typedef vector< shared_ptr<AuxiliaryVariableBase> > Variables;
shared_ptr<AuxiliaryVariableBase> findaux(Variables& aux, string name)
{
	for (size_t i=0; i<aux.size(); i++)
	{
		if (aux[i]->name == name) return aux[i];
	}
	return shared_ptr<AuxiliaryVariableBase>();
}


template <typename VAR>
typename ExPtr<VAR>::type createExpression(const Node* node, Variables& aux)
{
	if (node->type() == "expression")
	{
		assert(node->size() % 2 == 1);
		if (node->size() == 1) return createExpression<VAR>(node->child(0), aux);

		// process sub-expressions
		size_t symbols = node->size() / 2;
		vector<typename ExPtr<VAR>::type> expression(symbols + 1);
		vector<string> symbol(symbols);
		for (size_t i=0; i<=symbols; i++) expression[i] = createExpression<VAR>(node->child(2 * i), aux);
		for (size_t i=0; i<symbols; i++) symbol[i] = node->child(2 * i + 1)->value();

		// resolve operators in order of precedence
		for (size_t i=0; i<symbols; i++)
		{
			if (symbol[i] == "^")
			{
				typename ExPtr<VAR>::type lhs = expression[i];
				typename ExPtr<VAR>::type rhs = expression[i + 1];
				if (! isScalar<VAR>(lhs) || ! isScalar<VAR>(rhs)) throw runtime_error("operator ^ requires scalar operands");
				typename ExPtrT<double, VAR>::type p(new Power<VAR>(asScalar<VAR>(lhs), asScalar<VAR>(rhs)));
				if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
				symbol.erase(symbol.begin() + i);
				expression.erase(expression.begin() + i);
				expression[i] = p;
				i--; symbols--;
			}
			else if (symbol[i] == ".^")
			{
				typename ExPtr<VAR>::type lhs = expression[i];
				typename ExPtr<VAR>::type rhs = expression[i + 1];
				if (! isVector<VAR>(lhs)) throw runtime_error("operator .^ requires vectorial left hand side operand");
				if (isScalar<VAR>(rhs))
				{
					typename ExPtrT<Vector, VAR>::type p(new ElemPower<double, VAR>(asVector<VAR>(lhs), asScalar<VAR>(rhs)));
					if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
					symbol.erase(symbol.begin() + i);
					expression.erase(expression.begin() + i);
					expression[i] = p;
					i--; symbols--;
				}
				else
				{
					typename ExPtrT<Vector, VAR>::type p(new ElemPower<Vector, VAR>(asVector<VAR>(lhs), asVector<VAR>(rhs)));
					if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
					symbol.erase(symbol.begin() + i);
					expression.erase(expression.begin() + i);
					expression[i] = p;
					i--; symbols--;
				}
			}
		}
		for (size_t i=0; i<symbols; i++)
		{
			if (symbol[i] == "*")
			{
				typename ExPtr<VAR>::type lhs = expression[i];
				typename ExPtr<VAR>::type rhs = expression[i + 1];
				if (isScalar<VAR>(lhs))
				{
					if (isScalar<VAR>(rhs))
					{
						typename ExPtrT<double, VAR>::type p(new Product<double, double, double, VAR>(asScalar<VAR>(lhs), asScalar<VAR>(rhs)));
						if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
						symbol.erase(symbol.begin() + i);
						expression.erase(expression.begin() + i);
						expression[i] = p;
						i--; symbols--;
					}
					else
					{
						typename ExPtrT<Vector, VAR>::type p(new Product<double, Vector, Vector, VAR>(asScalar<VAR>(lhs), asVector<VAR>(rhs)));
						if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
						symbol.erase(symbol.begin() + i);
						expression.erase(expression.begin() + i);
						expression[i] = p;
						i--; symbols--;
					}
				}
				else
				{
					if (isScalar<VAR>(rhs))
					{
						typename ExPtrT<Vector, VAR>::type p(new Product<Vector, double, Vector, VAR>(asVector<VAR>(lhs), asScalar<VAR>(rhs)));
						if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
						symbol.erase(symbol.begin() + i);
						expression.erase(expression.begin() + i);
						expression[i] = p;
						i--; symbols--;
					}
					else
					{
						typename ExPtrT<double, VAR>::type p(new Product<Vector, Vector, double, VAR>(asVector<VAR>(lhs), asVector<VAR>(rhs)));
						if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
						symbol.erase(symbol.begin() + i);
						expression.erase(expression.begin() + i);
						expression[i] = p;
						i--; symbols--;
					}
				}
			}
			else if (symbol[i] == ".*")
			{
				typename ExPtr<VAR>::type lhs = expression[i];
				typename ExPtr<VAR>::type rhs = expression[i + 1];
				if (! isVector<VAR>(lhs) || ! isVector<VAR>(rhs)) throw runtime_error("operator .* requires vectorial operands");
				typename ExPtrT<Vector, VAR>::type p(new ElemProduct<VAR>(asVector<VAR>(lhs), asVector<VAR>(rhs)));
				if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
				symbol.erase(symbol.begin() + i);
				expression.erase(expression.begin() + i);
				expression[i] = p;
				i--; symbols--;
			}
			else if (symbol[i] == "/")
			{
				typename ExPtr<VAR>::type lhs = expression[i];
				typename ExPtr<VAR>::type rhs = expression[i + 1];
				if (! isScalar<VAR>(rhs)) throw runtime_error("operator / requires scalar right hand side operand");
				if (isScalar<VAR>(lhs))
				{
					typename ExPtrT<double, VAR>::type p(new Quotient<double, VAR>(asScalar<VAR>(lhs), asScalar<VAR>(rhs)));
					if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
					symbol.erase(symbol.begin() + i);
					expression.erase(expression.begin() + i);
					expression[i] = p;
					i--; symbols--;
				}
				else
				{
					typename ExPtrT<Vector, VAR>::type p(new Quotient<Vector, VAR>(asVector<VAR>(lhs), asScalar<VAR>(rhs)));
					if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
					symbol.erase(symbol.begin() + i);
					expression.erase(expression.begin() + i);
					expression[i] = p;
					i--; symbols--;
				}
			}
			else if (symbol[i] == "./")
			{
				typename ExPtr<VAR>::type lhs = expression[i];
				typename ExPtr<VAR>::type rhs = expression[i + 1];
				if (! isVector<VAR>(lhs) || ! isScalar<VAR>(rhs)) throw runtime_error("operator .* requires vectorial operands");
				typename ExPtrT<Vector, VAR>::type p(new ElemQuotient<VAR>(asVector<VAR>(lhs), asVector<VAR>(rhs)));
				if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
				symbol.erase(symbol.begin() + i);
				expression.erase(expression.begin() + i);
				expression[i] = p;
				i--; symbols--;
			}
		}
		for (size_t i=0; i<symbols; i++)
		{
			if (symbol[i] == "+")
			{
				typename ExPtr<VAR>::type lhs = expression[i];
				typename ExPtr<VAR>::type rhs = expression[i + 1];
				if (isScalar<VAR>(lhs) != isScalar<VAR>(rhs)) throw runtime_error("operator + cannot mix scalar and vectorial operands");
				if (isScalar<VAR>(lhs))
				{
					typename ExPtrT<double, VAR>::type p(new Sum<double, VAR>(asScalar<VAR>(lhs), asScalar<VAR>(rhs)));
					if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
					symbol.erase(symbol.begin() + i);
					expression.erase(expression.begin() + i);
					expression[i] = p;
					i--; symbols--;
				}
				else
				{
					typename ExPtrT<Vector, VAR>::type p(new Sum<Vector, VAR>(asVector<VAR>(lhs), asVector<VAR>(rhs)));
					if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
					symbol.erase(symbol.begin() + i);
					expression.erase(expression.begin() + i);
					expression[i] = p;
					i--; symbols--;
				}
			}
			else if (symbol[i] == "-")
			{
				typename ExPtr<VAR>::type lhs = expression[i];
				typename ExPtr<VAR>::type rhs = expression[i + 1];
				if (isScalar<VAR>(lhs) != isScalar<VAR>(rhs)) throw runtime_error("operator - cannot mix scalar and vectorial operands");
				if (isScalar<VAR>(lhs))
				{
					typename ExPtrT<double, VAR>::type p(new Difference<double, VAR>(asScalar<VAR>(lhs), asScalar<VAR>(rhs)));
					if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
					symbol.erase(symbol.begin() + i);
					expression.erase(expression.begin() + i);
					expression[i] = p;
					i--; symbols--;
				}
				else
				{
					typename ExPtrT<Vector, VAR>::type p(new Difference<Vector, VAR>(asVector<VAR>(lhs), asVector<VAR>(rhs)));
					if (isConstant<VAR>(lhs) && isConstant<VAR>(rhs)) p = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
					symbol.erase(symbol.begin() + i);
					expression.erase(expression.begin() + i);
					expression[i] = p;
					i--; symbols--;
				}
			}
		}
		assert(symbols == 0);
		return expression[0];
	}
	else if (node->type() == "negation")
	{
		assert(node->size() == 2);
		assert(node->child(0)->type() == "symbol");
		assert(node->child(0)->value() == "-");
		typename ExPtr<VAR>::type rhs = createExpression<VAR>(node->child(1), aux);
		if (isScalar<VAR>(rhs))
		{
			typename ExPtrT<double, VAR>::type p = typename ExPtrT<double, VAR>::type(new Negation<double, VAR>(asScalar<VAR>(rhs)));
			if (isConstant<VAR>(rhs)) return typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
			else return p;
		}
		else
		{
			typename ExPtrT<Vector, VAR>::type p = typename ExPtrT<Vector, VAR>::type(new Negation<Vector, VAR>(asVector<VAR>(rhs)));
			if (isConstant<VAR>(rhs)) return typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
			else return p;
		}
	}
	else if (node->type() == "simple expression")
	{
		// process access operators
		typename ExPtr<VAR>::type baseex = createExpression<VAR>(node->child(0), aux);
		if (node->size() == 1) return baseex;
		if (! isVector<VAR>(baseex)) throw runtime_error("cannot access entry or range of scalar");
		typename ExPtrT<Vector, VAR>::type ex = asVector<VAR>(baseex);
		bool ex_const = isConstant<VAR>(ex);
		if (ex_const)
		{
			ex = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(ex->eval(0.0)));
		}
		for (size_t i=1; i<node->size(); i++)
		{
			const Node* xsnode = node->child(i);
			if (xsnode->size() == 1)
			{
				assert(xsnode->type() == "vector entry");
				assert(i == node->size() - 1);
				typename ExPtr<VAR>::type index = createExpression<VAR>(xsnode->child(0), aux);
				if (! isScalar<VAR>(index)) throw runtime_error("vector index must be scalar");
				typename ExPtrT<double, VAR>::type p(new VectorEntry<VAR>(ex, asScalar<VAR>(index)));
				if (ex_const && isConstant<VAR>(index))
				{
					return typename ExPtr<VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
				}
				else
				{
					return p;
				}
			}
			else
			{
				assert(xsnode->type() == "vector range");
				assert(xsnode->size() == 2);
				typename ExPtr<VAR>::type begin = createExpression<VAR>(xsnode->child(0), aux);
				typename ExPtr<VAR>::type end = createExpression<VAR>(xsnode->child(1), aux);
				if (! isScalar<VAR>(begin)) throw runtime_error("vector index must be scalar");
				if (! isScalar<VAR>(end)) throw runtime_error("vector index must be scalar");
				typename ExPtrT<Vector, VAR>::type p(new VectorRange<VAR>(ex, asScalar<VAR>(begin), asScalar<VAR>(end)));
				if (ex_const && isConstant<VAR>(begin) && isConstant<VAR>(end))
				{
					ex = typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
				}
				else
				{
					ex_const = false;
					ex = p;
				}
			}
		}
		return ex;
	}
	else if (node->type() == "component-wise operation")
	{
		assert(node->size() == 2);
		typename ExPtr<VAR>::type arg = createExpression<VAR>(node->child(0), aux);
		if (! isVector<VAR>(arg)) throw runtime_error("component-wise operation cannot be applied to scalar");
		typename ExPtr<double>::type oper = createExpression<double>(node->child(1), aux);
		if (! isScalar<double>(oper)) throw runtime_error("component-wise operation must be scalar-valued");
		return typename ExPtrT<Vector, VAR>::type(new ComponentWiseOperation<VAR>(asVector<VAR>(arg), asScalar<double>(oper)));
	}
	else if (node->type() == "built-in function")
	{
		assert(node->size() == 1);
		string funcname = node->value();
		typename ExPtr<VAR>::type arg = createExpression<VAR>(node->child(0), aux);
		if (isScalar<VAR>(arg))
		{
			typename ExPtrT<double, VAR>::type sarg = asScalar<VAR>(arg);
			if (funcname == "zeros" || funcname == "ones" || funcname == "range")
			{
				ExpressionT<Vector, VAR>* f = NULL;
				if (funcname == "zeros") f = new ConstVect<VAR>(sarg, 0.0);
				else if (funcname == "ones") f = new ConstVect<VAR>(sarg, 1.0);
				else if (funcname == "range") f = new RangeVect<VAR>(sarg);
				typename ExPtrT<Vector, VAR>::type p(f);
				if (isConstant<VAR>(sarg)) return typename ExPtrT<Vector, VAR>::type(new Constant<Vector, VAR>(p->eval(0.0)));
				else return p;
			}
			else
			{
				ExpressionT<double, VAR>* f = NULL;
				if (funcname == "abs") f = new Function<VAR>(abs, sarg);
				else if (funcname == "floor") f = new Function<VAR>(floor, sarg);
				else if (funcname == "ceil") f = new Function<VAR>(ceil, sarg);
				else if (funcname == "round") f = new Function<VAR>(round, sarg);
				else if (funcname == "sqr") f = new Function<VAR>(sqr, sarg);
				else if (funcname == "sqrt") f = new Function<VAR>(sqrt, sarg);
				else if (funcname == "exp") f = new Function<VAR>(exp, sarg);
				else if (funcname == "log") f = new Function<VAR>(log, sarg);
				else if (funcname == "log10") f = new Function<VAR>(log10, sarg);
				else if (funcname == "sin") f = new Function<VAR>(sin, sarg);
				else if (funcname == "cos") f = new Function<VAR>(cos, sarg);
				else if (funcname == "tan") f = new Function<VAR>(tan, sarg);
				else if (funcname == "sinh") f = new Function<VAR>(sinh, sarg);
				else if (funcname == "cosh") f = new Function<VAR>(cosh, sarg);
				else if (funcname == "tanh") f = new Function<VAR>(tanh, sarg);
				else if (funcname == "asin") f = new Function<VAR>(asin, sarg);
				else if (funcname == "acos") f = new Function<VAR>(acos, sarg);
				else if (funcname == "atan") f = new Function<VAR>(atan, sarg);
				else throw runtime_error("function " + funcname + " cannot be applied to scalar argument");
				typename ExPtrT<double, VAR>::type p(f);
				if (isConstant<VAR>(sarg)) return typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
				else return p;
			}
		}
		else
		{
			typename ExPtrT<Vector, VAR>::type varg = asVector<VAR>(arg);
			ExpressionT<double, VAR>* f = NULL;
			if (funcname == "dim") f = new Dimension<VAR>(varg);
			else if (funcname == "sum") f = new SumAggregation<VAR>(varg);
			else if (funcname == "prod") f = new ProductAggregation<VAR>(varg);
			else if (funcname == "norm") f = new TwoNorm<VAR>(varg);
			else if (funcname == "sqrnorm") f = new TwoNorm2<VAR>(varg);
			else if (funcname == "min") f = new MinAggregation<VAR>(varg);
			else if (funcname == "max") f = new MaxAggregation<VAR>(varg);
			else throw runtime_error("function " + funcname + " cannot be applied to vector argument");
			typename ExPtrT<double, VAR>::type p(f);
			if (isConstant<VAR>(varg)) return typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(p->eval(0.0)));
			else return p;
		}
	}
	else if (node->type() == "vector composition")
	{
		shared_ptr< VectorComposition<VAR> > comp(new VectorComposition<VAR>());
		for (size_t i=0; i<node->size(); i++)
		{
			typename ExPtr<VAR>::type sub = createExpression<VAR>(node->child(i), aux);
			comp->add(sub);
		}
		return comp;
	}
	else if (node->type() == "bracket expression")
	{
		return createExpression<VAR>(node->child(0), aux);
	}
	else if (node->type() == "floatingpoint")
	{
		double value = strtod(node->value().c_str(), NULL);
		return typename ExPtrT<double, VAR>::type(new Constant<double, VAR>(value));
	}
	else if (node->type() == "identifier")
	{
		string varname = node->value();
		shared_ptr<AuxiliaryVariableBase> var = findaux(aux, varname);
		if (varname == "x")
		{
			if (ParameterTraits<VAR>::isScalar) throw runtime_error("variable 'x' cannot be used in second argument of apply statement");
			return typename ExPtrT<VAR, VAR>::type(new Variable<VAR>());
		}
		else if (varname == "lambda")
		{
			if (ParameterTraits<VAR>::isVector) throw runtime_error("variable 'lambda' cannot be used outside second argument of apply statement");
			return typename ExPtrT<VAR, VAR>::type(new Variable<VAR>());
		}
		else if (var != NULL)
		{
			if (isScalar(var)) return typename ExPtrT<double, VAR>::type(new VariableReference<double, VAR>(asScalar(var)));
			else return typename ExPtrT<Vector, VAR>::type(new VariableReference<Vector, VAR>(asVector(var)));
		}
		else throw runtime_error("unknown variable: '" + varname + "'");
	}
	else
	{
		throw runtime_error("internal error");
	}
}

struct Expression : public ExpressionT<double, Vector>
{
	Expression(ExPtrT<double, Vector>::type ex_, Variables& aux_)
	: ex(ex_)
	, aux(aux_)
	{ }

	double eval(Vector const& x) const
	{
		for (size_t i=0; i<aux.size(); i++)
		{
			aux[i]->preeval(x);
		}
		for (size_t i=0; i<aux.size(); i++)
		{
			shared_ptr<AuxiliaryVariableBase> var = aux[i];
			if (isScalar(var)) asScalar(var)->eval();
			else asVector(var)->eval();
		}
		return ex->eval(x);
	}

	ExPtrT<double, Vector>::type ex;
	Variables aux;
};

// interface function
ExpressionPtr parse(string str)
{
	// scan and parse
	ExpressionParser parser;
	const Node* node = parser.parse(str);
	assert(node->size() == 1 && node->child(0)->type() == "root");
	node = node->child(0);

	// create auxiliary variables
	Variables aux;
	for (size_t i=1; i<node->size(); i++)
	{
		const Node* sub = node->child(i-1);
		assert(sub->type() == "auxiliary variable definition");
		string varname = sub->child(0)->value();
		for (size_t j=0; j<aux.size(); j++)
		{
			if (aux[j]->name == varname) throw runtime_error("re-definition of variable '" + varname + "'");
		}
		ExPtr<Vector>::type ex = createExpression<Vector>(sub->child(1), aux);
		if (isScalar<Vector>(ex))
			aux.push_back(shared_ptr<AuxiliaryVariableBase>(new SAuxiliaryVariable(varname, asScalar<Vector>(ex))));
		else
			aux.push_back(shared_ptr<AuxiliaryVariableBase>(new VAuxiliaryVariable(varname, asVector<Vector>(ex))));
	}

	// create final expression
	const Node* sub = node->child(node->size() - 1);
	ExPtr<Vector>::type ex = createExpression<Vector>(sub, aux);
	if (! isScalar<Vector>(ex)) throw runtime_error("expression result must be scalar");

	// create encapsulation
	ExpressionPtr ret(new Expression(asScalar<Vector>(ex), aux));

	return ret;
}

// interface function
double evaluate(ExpressionPtr ex, Vector const& x)
{
	return ex->eval(x);
}
