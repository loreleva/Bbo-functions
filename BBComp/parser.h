
#pragma once


////////////////////////////////////////////////////////////
// Parser emitting an abstract syntax tree.
//


#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <set>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <memory>

#if _MSC_VER >= 1800 // MSVC >=2013
	#define TokenSource tokenSource //TokenSource is already defined in winnt.h 
#endif

////////////////////////////////////////////////////////////
// Token description. This class is the basis of
// all communication between scanners and parsers.
//
class Token
{
public:
	Token();
	Token(std::string _token,
			const char* _type,
			std::size_t _line);
	Token(Token const& other);
	~Token();

	void operator = (Token const& rhs);

	inline bool operator == (std::string const& rhs) const
	{ return (m_token == rhs); }

	inline bool operator != (std::string const& rhs) const
	{ return (m_token != rhs); }

	// Return the token type (one of few types defined by the scanner).
	inline std::string type() const
	{ return ((m_type != NULL) ? std::string(m_type) : std::string()); }

	// Return the token value. This is the literal token definition
	// as it appears in the input stream.
	inline std::string value() const
	{ return m_token; }

	// Return the line in which the token definition starts.
	inline std::size_t line() const
	{ return m_line; }

	// Check whether this is the END OF INPUT dummy token
	inline bool isEnd() const
	{ return (m_token.empty()); }

	// raw token data - only scanners should access this, if at all:
	std::string m_token;					// "raw" token value (including quotes, etc.)
	const char* m_type;						// for example "identifier"
	std::size_t m_line;					    // line of the first character of the token
};


////////////////////////////////////////////////////////////
// A TokenSource provides a stream of tokens.
// Typically, tokens come from a single input
// file or from a project file describing
// which files to scan.
//
// A sub-class of TokenSource must fill in the
// m_token list and append an "end" marker, in
// the form of an empty (default-constructed)
// token.
//
class TokenSource
{
public:
	typedef std::vector<Token> Container;
	typedef Container::const_iterator const_iterator;

	inline std::size_t size() const
	{ return m_token.size(); }
	inline Token const& operator [] (std::size_t index) const
	{ return m_token[index]; }
	inline const_iterator begin() const
	{ return m_token.begin(); }
	inline const_iterator end() const
	{ return m_token.end(); }

protected:
	Container m_token;
};


typedef TokenSource::const_iterator TokenIter;


// C-like rules for whitespace and
// identifiers, can handle keywords.
class DefaultScanner : public TokenSource
{
public:
	DefaultScanner(const char* linecomment = "//", bool defaultTokens = true);
	~DefaultScanner();

	inline void addKeyword(std::string keyword)
	{ m_keyword.insert(keyword); }
	inline void addToken(std::string token)
	{ m_other.insert(m_other.begin(), token); }

	bool scan(std::string content, std::stringstream& error);

	// on failure to interpret escape sequence: return false, leave it and value in an undefined state
	// of success to interpret escape sequence: return true, set iterator to end (back + 1) of the sequence, set value to the escaped character code
	static bool scanEscapeSequence(const char*& it, unsigned int& value);

protected:
	inline static bool starts(const char* string, const char* key)
	{ return (memcmp(string, key, strlen(key)) == 0); }
	inline static bool isHex(char c)
	{ return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')); }
	inline static unsigned int asHex(char c)
	{
		if (c >= '0' && c <= '9') return (c - '0');
		if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
		if (c >= 'a' && c <= 'f') return (c - 'a' + 10);
		throw std::runtime_error("[DefaultScanner::asHex] invalid input character");
	}

	const char* m_linecomment;
	std::set<std::string> m_keyword;
	std::vector<std::string> m_other;
};


////////////////////////////////////////////////////////////
// Parse (Sub-)Tree.
// A Parse-Tree is the output of the parser in case of success.
//
class Node
{
public:
	typedef std::vector<Node*> Container;
	typedef Container::iterator iterator;
	typedef Container::const_iterator const_iterator;

	Node(TokenIter position);
	~Node();

	// const methods for reading the syntax tree
	inline const Token* token() const
	{ return (const Token*)(&(*m_token)); }
	inline const Token* operator () () const
	{ return token(); }
	inline iterator begin()
	{ return m_child.begin(); }
	inline const_iterator begin() const
	{ return m_child.begin(); }
	inline iterator end()
	{ return m_child.end(); }
	inline const_iterator end() const
	{ return m_child.end(); }
	inline std::size_t size() const
	{ return m_child.size(); }
	inline const Node* child(std::size_t index) const
	{
#ifdef DEBUG
		if (index >= m_child.size()) throw std::runtime_error("[Node::child] index out of bounds");
#endif
		return m_child[index];
	}
	inline Node const& parent () const
	{ return *m_parent; }
	inline Node const& operator [] (std::size_t index) const
	{ return *child(index); }
	inline Node const& operator () (const char* field) const
	{ return *child(findIndex(field)); }
	inline std::string type() const
	{ return ((m_type != NULL) ? std::string(m_type) : std::string()); }
	inline std::string value() const
	{ return (*m_token).value(); }
	std::size_t findIndex(const char* field) const;

	// non-const methods for constructing the tree
	inline void setType(const char* type)
	{ m_type = type; }
	inline void addChild(Node* child)
	{ m_child.push_back(child); child->m_parent = this; }
	void merge(Node* other);

	void prettyprint(std::ostream& os, std::size_t indent = 0) const;

protected:
	Node* m_parent;
	Container m_child;
	const char* m_type;				// typically rule name
	TokenIter m_token;				// start token
};


////////////////////////////////////////////////////////////
// Error reporting
//
class ErrorMessage
{
public:
	static std::string format(const Token* token, std::string message);
	static std::string format(const Node* node, std::string message);
};


////////////////////////////////////////////////////////////
// parsing result and error description objects
//


enum ParseStatus
{
	undefined = 99,		// not set yet
	success = 1,        // string is valid
	failure = 0,        // string is invalid
	fatal = -1,         // string is not valid, although a keyword suggests so
};


class ParseResult
{
public:
	ParseResult();
	ParseResult(ParseResult const& other);

	inline ParseStatus status() const
	{ return m_status; }

	inline std::string errorMessage() const
	{
		const Token* token = &*m_token;
		std::string message = m_what;
		if (m_context != "") message += " " + m_context;
		return ErrorMessage::format(token, message);
	}

	inline void setSuccess()
	{
		m_status = success;
		m_token = TokenIter();
		m_what = "";
		m_context = "";
	}

	inline void setFailure(TokenIter token, std::string what, std::string context = "")
	{
		m_status = failure;
		m_token = token;
		m_what = what;
		m_context = context;
	}

	ParseStatus m_status;			// outcome of parsing

	// error:
	TokenIter m_token;				// start of error region
	std::string m_what;				// what went wrong
	std::string m_context;			// in which context?
};


////////////////////////////////////////////////////////////
// Abstract base class of all parsers
//
class AbstractParserBase
{
public:
	AbstractParserBase();
	virtual ~AbstractParserBase();

	virtual TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const = 0;

	Node* parseAll(TokenSource const& tokens, std::string& error) const;
};


////////////////////////////////////////////////////////////
// Wrapper class for parsers.
// This class acts like a parser, but it actually manages
// a parser object in either a plain pointer or a smart
// pointer.
//
class Parser
{
public:
	Parser();
	Parser(bool smart, AbstractParserBase* parser);
	Parser(std::shared_ptr<AbstractParserBase> parser);
	Parser(Parser const& other);
	virtual ~Parser();

	void operator = (Parser const& other);

	virtual TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;
	Node* parseAll(TokenSource const& tokens, std::string& error) const;

	template <typename T>
	T* as()
	{
		if (m_smart)
		{
			return dynamic_cast<T*>(m_shared.get());
		}
		else
		{
			return dynamic_cast<T*>(m_plain);
		}
	}

protected:
	bool m_smart;
	AbstractParserBase* m_plain;
	std::shared_ptr<AbstractParserBase> m_shared;
};


////////////////////////////////////////////////////////////
// Named rule; creates a named node in the parse tree.
// A rule is assumed to be destroyed by an enclosing parser
// object, hence it should not be stored in a smart pointer.
//
class Rule : public AbstractParserBase
{
public:
	Rule(const char* nodename);

	const char* nodename() const
	{ return m_nodename; }

	void operator = (Parser const& parser);

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

	operator Parser ()
	{ return Parser(false, this); }

protected:
	const char* m_nodename;
	Parser m_parser;
};


////////////////////////////////////////////////////////////
// "hand-inserted" node in the parse tree:
// On acceptance, this parser inserts a node into the
// parse tree. It redirects the output of another parser
// into this sub-tree. The inserted node does not need to
// correspond to input stream tokens.
//
class Block : public AbstractParserBase
{
public:
	Block(Parser parser)
	: m_type(NULL)
	, m_parser(parser)
	{ }
	Block(const char* type, Parser parser)
	: m_type(type)
	, m_parser(parser)
	{ }
	Block(Block const& other);
	~Block();

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	const char* m_type;
	Parser m_parser;
};


Parser block(const char* name, Parser parser);


////////////////////////////////////////////////////////////
// "hand-inserted" node in the parse tree.
// This parser always accepts, and it does not consume any
// input tokens. It always inserts a fixed terminal node
// (the marker) into the parse tree. This allows for the
// insertion of terminals that do not actually appear in
// the input stream.
//
class Marker : public AbstractParserBase
{
public:
	Marker(const char* type)
	: m_type(type)
	{ }
	Marker(Marker const& other);
	~Marker();

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	const char* m_type;
};


Parser marker(const char* name);


////////////////////////////////////////////////////////////
// Parser wrapper discarding all output.
//
class Discard : public AbstractParserBase
{
public:
	Discard(Parser parser)
	: m_parser(parser)
	{ }
	Discard(Discard const& other);
	~Discard();

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	Parser m_parser;
};


////////////////////////////////////////////////////////////
// Parser recognizing the empty string, thus,
// accepting everything. Mostly useless.
//
class EpsilonParser : public AbstractParserBase
{
public:
	EpsilonParser();
	EpsilonParser(EpsilonParser const& other);

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;
};

extern Parser eps;


////////////////////////////////////////////////////////////
// Parser recognizing a given literal.
//
class LiteralParser : public AbstractParserBase
{
public:
	LiteralParser(std::string terminal, bool output = false, const char* type = NULL, bool invertTypeRule = false);
	LiteralParser(LiteralParser const& other);

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

	inline std::string terminal() const
	{ return m_terminal; }

protected:
	std::string m_terminal;
	bool m_output;
	const char* m_type;
	bool m_invertTypeRule;
};


////////////////////////////////////////////////////////////
// Convenience method for literals
//
Parser lit(const char* literal, bool output = false);   // scan arbitrary literal
Parser key(const char* keyword, bool output = false);   // scan keyword literal
Parser sym(const char* keyword, bool output = false);   // scan non-keyword literal


////////////////////////////////////////////////////////////
// Parser for named token types
//
class TokenTypeParser : public AbstractParserBase
{
public:
	TokenTypeParser(const char* tokentype, const char* converttype = NULL);
	TokenTypeParser(TokenTypeParser const& other);

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	const char* m_tokentype;
	const char* m_converttype;
};

extern Parser identifier;
extern Parser integer;
extern Parser floatingpoint;
extern Parser singlequoted;
extern Parser doublequoted;


////////////////////////////////////////////////////////////
// Parser for lists of predefined tokens,
// such as keywords or operators.
//
class SymbolTableParser : public AbstractParserBase
{
public:
	SymbolTableParser(std::string name);
	SymbolTableParser(SymbolTableParser const& other);

	void add(std::string symbol);

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	std::string m_name;
	std::set<std::string> m_symbol;
};

Parser symboltable(std::string name, std::string init, std::string separator = " ");


////////////////////////////////////////////////////////////
// A ForceApplication object can be inserted into any
// sequence of parsers (see SerialParser), but not as
// the first element. It turns a simple failure of
// parsing on the right of this marker into a fatal
// failure. This is useful for cutting long branches of
// the search tree, typically after a keyword. The
// mechanism is also valuable for quality error reporting.
//
class ForceApplication
{
public:
	ForceApplication();
	ForceApplication(ForceApplication const& other);
	virtual ~ForceApplication();   // virtual; so the type can be inferred dynamically
};

extern ForceApplication applies;


////////////////////////////////////////////////////////////
// Serial combination of two parsers
//
class SerialParser : public AbstractParserBase
{
public:
	SerialParser();
	SerialParser(SerialParser const& other);
	~SerialParser();

	inline void add(Parser const& parser)
	{ m_sub.push_back(new Parser(parser)); }
	inline void add(ForceApplication force)
	{ m_sub.push_back(NULL); }

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	std::list<Parser*> m_sub;
};


////////////////////////////////////////////////////////////
// Convenience operators denoting serial combinations
//

Parser operator >> (Parser lhs, Parser rhs);
Parser operator >> (Parser lhs, const char* rhs);
Parser operator >> (const char* lhs, Parser rhs);
Parser operator >> (Parser lhs, ForceApplication rhs);
Parser operator >> (const char* lhs, ForceApplication rhs);


////////////////////////////////////////////////////////////
// rule repetition
//
enum RepetitionParserType
{
	rptOptional,
	rptZeroOrMore,
	rptOneOrMore,
};
class RepetitionParser : public AbstractParserBase
{
public:
	RepetitionParser(Parser parser, RepetitionParserType type)
	: m_parser(parser)
	, m_type(type)
	{ }

	RepetitionParser(RepetitionParser const& other);
	~RepetitionParser();

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	Parser m_parser;
	RepetitionParserType m_type;
};


////////////////////////////////////////////////////////////
// Convenience operators denoting options and repetitions
//
Parser operator - (Parser parser);
Parser operator * (Parser parser);
Parser operator + (Parser parser);


////////////////////////////////////////////////////////////
// alternatives
//
class AlternativeParser : public AbstractParserBase
{
public:
	AlternativeParser();
	AlternativeParser(AlternativeParser const& other);
	~AlternativeParser();

	inline void add(Parser const& parser)
	{ m_sub.push_back(parser); }

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	std::list<Parser> m_sub;
};


////////////////////////////////////////////////////////////
// Convenience operators for alternatives
//
Parser operator | (Parser lhs, Parser rhs);
Parser operator | (Parser lhs, const char* rhs);
Parser operator | (const char* lhs, Parser rhs);


////////////////////////////////////////////////////////////
// lists
//
class ListParser : public AbstractParserBase
{
public:
	ListParser(Parser content, Parser delimiter, bool emptyAllowed);
	ListParser(ListParser const& other);
	~ListParser();

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	Parser m_content;
	Parser m_delimiter;
	bool m_emptyAllowed;
};


////////////////////////////////////////////////////////////
// Convenience operators for lists
//
Parser operator % (Parser lhs, Parser rhs);
Parser operator % (Parser lhs, const char* rhs);
Parser operator / (Parser lhs, Parser rhs);
Parser operator / (Parser lhs, const char* rhs);


////////////////////////////////////////////////////////////
// difference parser
// - accept one but not the other
//
class DifferenceParser : public AbstractParserBase
{
public:
	DifferenceParser(Parser good, Parser bad)
	: m_good(good)
	, m_bad(bad)
	{ }
	DifferenceParser(DifferenceParser const& other);
	~DifferenceParser();

	TokenIter parse(TokenIter iter, Node* tree, ParseResult& result) const;

protected:
	Parser m_good;
	Parser m_bad;
};


////////////////////////////////////////////////////////////
// Convenience operators for differences
//
Parser operator - (Parser lhs, Parser rhs);
Parser operator - (Parser lhs, const char* rhs);
