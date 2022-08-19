
#include "parser.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <typeinfo>


using namespace std;


////////////////////////////////////////////////////////////


Token::Token()
: m_type(NULL)
, m_line(0)
{ }

Token::Token(string _token,
		const char* _type,
		size_t _line)
: m_token(_token)
, m_type(_type)
, m_line(_line)
{ }

Token::Token(Token const& other)
: m_token(other.m_token)
, m_type(other.m_type)
, m_line(other.m_line)
{ }

Token::~Token()
{ }


void Token::operator = (Token const& rhs)
{
	m_token = rhs.m_token;
	m_type = rhs.m_type;
	m_line = rhs.m_line;
}


////////////////////////////////////////////////////////////


DefaultScanner::DefaultScanner(const char* linecomment, bool defaultTokens)
: m_linecomment(linecomment)
{
	if (defaultTokens)
	{
		m_other.push_back(",");
		m_other.push_back(";");
		m_other.push_back("::");
		m_other.push_back(":");
		m_other.push_back(".");
		m_other.push_back("?");

		m_other.push_back("(");
		m_other.push_back(")");
		m_other.push_back("[");
		m_other.push_back("]");
		m_other.push_back("{");
		m_other.push_back("}");

		m_other.push_back("#");
		m_other.push_back("$");

		m_other.push_back("++");
		m_other.push_back("--");
		m_other.push_back("+=");
		m_other.push_back("-=");
		m_other.push_back("*=");
		m_other.push_back("/=");
		m_other.push_back("%=");
		m_other.push_back("^=");
		m_other.push_back("&&=");
		m_other.push_back("||=");
		m_other.push_back("&=");
		m_other.push_back("|=");
		m_other.push_back("<<=");
		m_other.push_back(">>=");

		m_other.push_back("&&");
		m_other.push_back("||");
		m_other.push_back("<<");
		m_other.push_back(">>");

		m_other.push_back("==");
		m_other.push_back("!=");
		m_other.push_back("~=");
		m_other.push_back("<>");
		m_other.push_back("<=");
		m_other.push_back(">=");
		m_other.push_back("<");
		m_other.push_back(">");

		m_other.push_back("+");
		m_other.push_back("-");
		m_other.push_back("*");
		m_other.push_back("/");
		m_other.push_back("%");
		m_other.push_back("^");
		m_other.push_back("!");
		m_other.push_back("&");
		m_other.push_back("|");
		m_other.push_back("~");

		m_other.push_back("=");
	}
}

DefaultScanner::~DefaultScanner()
{ }


bool DefaultScanner::scan(string content, stringstream& error)
{
	// scan the content
	const char* it = content.c_str();
	size_t line = 1;
	while (true)
	{
		char c = *it;

		// whitespace and comments
		if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r' || c == '\n')
		{
			// whitespace
			if (c == '\n') line++;
			it++;
			continue;
		}
		if (starts(it, m_linecomment))
		{
			// line comment
			it += strlen(m_linecomment);
			for (; *it != 0; it++)
			{
				if (*it == '\r' || *it == '\n')
				{
					if (*it == '\n') line++;
					it++;
					break;
				}
			}
			continue;
		}

		// different types of tokens
		const char* start = it;
		size_t ln = line;
		if (c == '\'')
		{
			// single quoted character constant
			it++;
			if (*it >= 32 || *it < 0)
			{
				if (*it == '\\')
				{
					unsigned int value;
					if (scanEscapeSequence(it, value))
					{
						if (*it == '\'')
						{
							it++;

							m_token.push_back(Token(
									string(start, it - start),
									"singlequoted",
									ln));
							continue;
						}
					}
				}
				else
				{
					it++;
					if (*it == '\'')
					{
						it++;

						m_token.push_back(Token(
								string(start, it - start),
								"singlequoted",
								ln));
						continue;
					}
				}
			}

			error << "line " << line << ": invalid character constant";
			return false;
		}
		if (c == '\"')
		{
			// double quoted character constant
			it++;
			while (true)
			{
				char c = *it;
				if (c == '\\')
				{
					unsigned int value;
					if (! scanEscapeSequence(it, value))
					{
						error << "line " << line << ": malformed escape sequence";
						return false;
					}
				}
				else if (c == '\"')
				{
					it++;
					break;
				}
				else if (c == 0)
				{
					error << "line " << line << ": string constant exceeds end of input";
					return false;
				}
				else it++;
			}
			m_token.push_back(Token(
					string(start, it - start),
					"doublequoted",
					ln));
			continue;
		}
		if ((c == '_')
				|| ((c >= 'A') && (c <= 'Z'))
				|| ((c >= 'a') && (c <= 'z')))
		{
			// identifier or keyword
			it++;
			while (true)
			{
				char c = *it;
				if (! ((c == '_')
					|| ((c >= '0') && (c <= '9'))
					|| ((c >= 'A') && (c <= 'Z'))
					|| ((c >= 'a') && (c <= 'z')))) break;
				it++;
			}
			Token tok(string(start, it - start), "identifier", ln);
			if (m_keyword.find(tok.value()) != m_keyword.end()) tok.m_type = "keyword";
			m_token.push_back(tok);
			continue;
		}
		if ((c >= '0') && (c <= '9'))
		{
			// numeric constant - integer or floatingpoint
			char* ipos = (char*)it;
			char* dpos = (char*)it;
			strtoll(it, &ipos, 0);
			strtold(it, &dpos);
			assert(ipos > it || dpos > it);
			if (ipos >= dpos)
			{
				// integer
				it = ipos;
				char c = *it;
				if (! ((c == '_') || (c == '.')
						|| ((c >= 'A') && (c <= 'Z'))
						|| ((c >= 'a') && (c <= 'z'))))
				{
					m_token.push_back(Token(
							string(start, it - start),
							"integer",
							ln));
					continue;
				}
			}
			else
			{
				// floatingpoint
				if (*(dpos-1) == '.')
				{
					error << "line " << line << ": floating point constant must not end with decimal point";
					return false;
				}
				it = dpos;
				char c = *it;
				if (! ((c == '_')
						|| ((c >= 'A') && (c <= 'Z'))
						|| ((c >= 'a') && (c <= 'z'))))
				{
					m_token.push_back(Token(
							string(start, it - start),
							"floatingpoint",
							ln));
					continue;
				}
			}
			it = start;
		}
		{
			// check for other tokens
			size_t t, tc = m_other.size();
			for (t=0; t<tc; t++)
			{
				if (starts(it, m_other[t].c_str()))
				{
					it += m_other[t].size();
					m_token.push_back(Token(
							string(start, it - start),
							"other",
							ln));
					break;
				}
			}
			if (t < tc) continue;
		}
		if (c == 0) break;

		// no token type fits
		error << "line " << line << ": invalid token at '";
		size_t i;
		for (i=0; i<10; i++)
		{
			if (*it == '\r' || *it == '\n' || *it == 0) break;
			error << *it;
			it++;
		}
		if (i == 10) error << "...";
		error << "'";
		return false;
	}

	// add "end" marker token
	m_token.push_back(Token());

	return true;
}

// static
bool DefaultScanner::scanEscapeSequence(const char*& it, unsigned int& value)
{
	value = 0;
	assert(*it == '\\');
	it++;
	char c = *it;
	switch (c)
	{
		case '\'':
		case '\"':
		case '?':
		case '\\':
		{
			value = (unsigned char)c;
			it++;
			return true;
		}
		case '0':
		{
			value = 0;
			it++;
			return true;
		}
		case 'a':
		{
			value = '\a';
			it++;
			return true;
		}
		case 'b':
		{
			value = '\b';
			it++;
			return true;
		}
		case 'f':
		{
			value = '\f';
			it++;
			return true;
		}
		case 'n':
		{
			value = '\n';
			it++;
			return true;
		}
		case 'r':
		{
			value = '\r';
			it++;
			return true;
		}
		case 't':
		{
			value = '\t';
			it++;
			return true;
		}
		case 'v':
		{
			value = '\v';
			it++;
			return true;
		}
		case 'x':
		{
			it++;
			if (! isHex(*it)) return false;
			value = asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			return true;
		}
		case 'u':
		{
			it++;
			if (! isHex(*it)) return false;
			value = asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			return true;
		}
		case 'U':
		{
			it++;
			if (! isHex(*it)) return false;
			value = asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			if (! isHex(*it)) return false;
			value = 16 * value + asHex(*it);
			it++;
			return true;
		}
	}
	return false;
}


////////////////////////////////////////////////////////////


Node::Node(TokenIter token)
: m_parent(NULL)
, m_child()
, m_type("")
, m_token(token)
{ }

Node::~Node()
{
	iterator i = begin();
	iterator e = end();
	for (; i != e; ++i) delete *i;
}


// move all sub-nodes from the other node to this node
void Node::merge(Node* other)
{
	iterator i = other->begin();
	iterator e = other->end();
	for (; i != e; ++i)
	{
		m_child.push_back(*i);
		(*i)->m_parent = this;
	}
	other->m_child.clear();
}

size_t Node::findIndex(const char* field) const
{
	size_t i, ic = size();
	for (i=0; i<ic; i++) if (strcmp(m_child[i]->m_type, field) == 0) return i;

	throw runtime_error("[Node::findIndex] field not found");
	return 0;
}

void Node::prettyprint(ostream& os, size_t indent) const
{
	size_t i;
	for (i=0; i<indent; i++) os << "  ";
	os << "[" << type() << "]";
	string v = value();
	if (! v.empty()) os << "  '" << v << "'";
	os << endl;
	Node::const_iterator it = begin();
	Node::const_iterator e = end();
	for (; it != e; it++) (*it)->prettyprint(os, indent + 1);
}


////////////////////////////////////////////////////////////


ParseResult::ParseResult()
: m_status(undefined)
{ }

ParseResult::ParseResult(ParseResult const& other)
: m_status(other.m_status)
, m_token(other.m_token)
, m_what(other.m_what)
, m_context(other.m_context)
{ }


////////////////////////////////////////////////////////////


AbstractParserBase::AbstractParserBase()
{ }

AbstractParserBase::~AbstractParserBase()
{
//	cout << "\t\t\t[~AbstractParserBase]  this=" << this << "  type=" << typeid(this).name() << endl;
}


Node* AbstractParserBase::parseAll(TokenSource const& tokens, string& error) const
{
	TokenIter it = tokens.begin();

	Node* tree = new Node(it);
	ParseResult result;

	it = parse(it, tree, result);

	if (result.status() == success)
	{
		if (it->isEnd()) return tree;
		else
		{
			delete tree;
			result.setFailure(it, "syntax error");
			error = result.errorMessage();
			return NULL;
		}
	}
	else
	{
		delete tree;
		error = result.errorMessage();
		return NULL;
	}
}


////////////////////////////////////////////////////////////


Parser::Parser()
{ }

Parser::Parser(bool smart, AbstractParserBase* parser)
: m_smart(smart)
, m_plain(smart ? NULL : parser)
, m_shared(smart ? parser : NULL)
{ }

Parser::Parser(shared_ptr<AbstractParserBase> parser)
: m_smart(true)
, m_plain(NULL)
, m_shared(parser)
{ }

Parser::Parser(Parser const& other)
: m_smart(other.m_smart)
, m_plain(other.m_plain)
, m_shared(other.m_shared)
{ }

Parser::~Parser()
{ }


void Parser::operator = (Parser const& other)
{
	m_smart = other.m_smart;
	m_plain = other.m_plain;
	m_shared = other.m_shared;
}

TokenIter Parser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	if (m_smart)
	{
#ifdef DEBUG
		if (m_shared.get() == NULL)
		{
			throw runtime_error("[Parser::parse] parser not initialized");
		}
#endif
		return m_shared->parse(iter, tree, result);
	}
	else
	{
#ifdef DEBUG
		if (m_plain == NULL)
		{
			throw runtime_error("[Parser::parse] parser not initialized");
		}
#endif
		return m_plain->parse(iter, tree, result);
	}
}

Node* Parser::parseAll(TokenSource const& tokens, std::string& error) const
{
	if (m_shared) return m_shared->parseAll(tokens, error);
	else return m_plain->parseAll(tokens, error);
}


////////////////////////////////////////////////////////////


Rule::Rule(const char* nodename)
: m_nodename(nodename)
{ }

void Rule::operator = (Parser const& parser)
{
	m_parser = parser;
}

TokenIter Rule::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	Node* node = NULL;
	Node* tr = tree;
	if (m_nodename != NULL)
	{
		node = new Node(iter);
		node->setType(m_nodename);
		tr = node;
	}

	iter = m_parser.parse(iter, tr, result);

	if (m_nodename != NULL)
	{
		if (result.status() == success)
		{
			tree->addChild(node);
		}
		else
		{
			delete node;
		}
	}

	return iter;
}


////////////////////////////////////////////////////////////


Block::Block(Block const& other)
: m_type(other.m_type)
, m_parser(other.m_parser)
{ }

Block::~Block()
{ }


TokenIter Block::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	Node* node = new Node(iter);
	if (m_type != NULL) node->setType(m_type);

	iter = m_parser.parse(iter, node, result);
	if (result.status() == success) tree->addChild(node);
	else delete node;

	return iter;
}


Parser block(const char* name, Parser parser)
{ return Parser(true, new Block(name, parser)); }


////////////////////////////////////////////////////////////


Marker::Marker(Marker const& other)
: m_type(other.m_type)
{ }

Marker::~Marker()
{ }


TokenIter Marker::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	Node* node = new Node(iter);
	node->setType(m_type);
	tree->addChild(node);
	result.setSuccess();
	return iter;
}


Parser marker(const char* name)
{ return Parser(true, new Marker(name)); }


////////////////////////////////////////////////////////////


Discard::Discard(Discard const& other)
: m_parser(other.m_parser)
{ }

Discard::~Discard()
{ }


TokenIter Discard::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	Node tmp(iter);
	return m_parser.parse(iter, &tmp, result);
}


////////////////////////////////////////////////////////////


EpsilonParser::EpsilonParser()
{ }

EpsilonParser::EpsilonParser(EpsilonParser const& other)
{ }


TokenIter EpsilonParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	result.setSuccess();
	return iter;
}


Parser eps(true, new EpsilonParser());


////////////////////////////////////////////////////////////


LiteralParser::LiteralParser(string terminal, bool output, const char* type, bool invertTypeRule)
: m_terminal(terminal)
, m_output(output)
, m_type(type)
, m_invertTypeRule(invertTypeRule)
{ }

LiteralParser::LiteralParser(LiteralParser const& other)
: m_terminal(other.m_terminal)
, m_output(other.m_output)
, m_type(other.m_type)
, m_invertTypeRule(other.m_invertTypeRule)
{ }


TokenIter LiteralParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	bool success = false;

	Token const& token = *iter;
	if (token == m_terminal)
	{
		if (m_type != NULL)
		{
			bool eq = (strcmp(token.m_type, m_type) == 0);
			if (m_invertTypeRule != eq) success = true;
		}
		else success = true;
	}

	if (success)
	{
		result.setSuccess();
		if (m_output)
		{
			Node* node = new Node(iter);
			node->setType("literal");
			tree->addChild(node);
		}
		iter++;
	}
	else
	{
		result.setFailure(iter, "'" + m_terminal + "' expected");
	}

	return iter;
}


Parser lit(const char* literal, bool output)
{ return Parser(true, new LiteralParser(literal, output)); }

Parser key(const char* literal, bool output)
{ return Parser(true, new LiteralParser(literal, output, "keyword")); }

Parser sym(const char* literal, bool output)
{ return Parser(true, new LiteralParser(literal, output, "keyword", true)); }


////////////////////////////////////////////////////////////


TokenTypeParser::TokenTypeParser(const char* tokentype, const char* converttype)
: m_tokentype(tokentype)
, m_converttype(converttype == NULL ? tokentype : converttype)
{ }

TokenTypeParser::TokenTypeParser(TokenTypeParser const& other)
: m_tokentype(other.m_tokentype)
, m_converttype(other.m_converttype)
{ }


TokenIter TokenTypeParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	Token const& token = *iter;

	if (token.type() == m_tokentype)
	{
		result.setSuccess();
		Node* node = new Node(iter);
		node->setType(m_tokentype);
		tree->addChild(node);
		iter++;
	}
	else if (m_converttype != NULL && token.type() == m_converttype)
	{
		result.setSuccess();
		Node* node = new Node(iter);
		node->setType(m_tokentype);
		tree->addChild(node);
		iter++;
	}
	else
	{
		result.setFailure(iter, string(m_tokentype) + " expected");
	}
	return iter;
}

Parser identifier(true, new TokenTypeParser("identifier"));
Parser integer(true, new TokenTypeParser("integer"));
Parser floatingpoint(true, new TokenTypeParser("floatingpoint", "integer"));
Parser singlequoted(true, new TokenTypeParser("singlequoted"));
Parser doublequoted(true, new TokenTypeParser("doublequoted"));


////////////////////////////////////////////////////////////


SymbolTableParser::SymbolTableParser(string name)
: m_name(name)
{ }

SymbolTableParser::SymbolTableParser(SymbolTableParser const& other)
: m_name(other.m_name)
, m_symbol(other.m_symbol)
{ }


void SymbolTableParser::add(string symbol)
{
	m_symbol.insert(symbol);
}

TokenIter SymbolTableParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	string token = (*iter).value();
	set<string>::iterator pos = m_symbol.find(token);
	if (pos != m_symbol.end())
	{
		result.setSuccess();
		Node* node = new Node(iter);
		node->setType("symbol");
		tree->addChild(node);
		iter++;
		return iter;
	}

	result.setFailure(iter, m_name + " expected");
	return iter;
}

Parser symboltable(string name, string init, string separator)
{
	SymbolTableParser* s = new SymbolTableParser(name);
	if (! init.empty())
	{
		size_t start = 0;
		while (true)
		{
			size_t pos = init.find(separator, start);
			if (pos == string::npos)
			{
				s->add(init.substr(start));
				break;
			}
			else
			{
				s->add(init.substr(start, pos - start));
				start = pos + separator.size();
			}
		}
	}
	return Parser(true, s);
}


////////////////////////////////////////////////////////////


ForceApplication::ForceApplication()
{ }

ForceApplication::ForceApplication(ForceApplication const& other)
{ }

ForceApplication::~ForceApplication()
{ }


ForceApplication applies;


////////////////////////////////////////////////////////////


SerialParser::SerialParser()
{ }

SerialParser::SerialParser(SerialParser const& other)
{
	list<Parser*>::const_iterator it = other.m_sub.begin();
	list<Parser*>::const_iterator e = other.m_sub.end();
	for (; it != e; ++it)
	{
		if (*it == NULL) m_sub.push_back(NULL);
		else m_sub.push_back(new Parser(**it));
	}
}

SerialParser::~SerialParser()
{
	list<Parser*>::const_iterator it = m_sub.begin();
	list<Parser*>::const_iterator e = m_sub.end();
	for (; it != e; ++it)
	{
		if (*it != NULL) delete *it;
	}
}


TokenIter SerialParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	Node node(iter);
	bool bFatal = false;
	TokenIter start = iter;

	list<Parser*>::const_iterator it = m_sub.begin();
	list<Parser*>::const_iterator e = m_sub.end();
	for (; it != e; ++it)
	{
		if (*it == NULL) bFatal = true;
		else
		{
			iter = (*it)->parse(iter, &node, result);
			if (result.status() != success)
			{
				if (bFatal) result.m_status = fatal;
				return start;
			}
		}
	}

	result.setSuccess();
	tree->merge(&node);
	return iter;
}


Parser operator >> (Parser lhs, Parser rhs)
{
	SerialParser* s = lhs.as<SerialParser>();
	if (s != NULL)
	{
		s->add(rhs);
		return lhs;
	}
	else
	{
		s = new SerialParser();
		s->add(lhs);
		s->add(rhs);
		return Parser(true, s);
	}
}

Parser operator >> (Parser lhs, const char* rhs)
{
	SerialParser* s = lhs.as<SerialParser>();
	if (s != NULL)
	{
		s->add(Parser(true, new LiteralParser(rhs)));
		return lhs;
	}
	else
	{
		s = new SerialParser();
		s->add(lhs);
		s->add(Parser(true, new LiteralParser(rhs)));
		return Parser(true, s);
	}
}

Parser operator >> (const char* lhs, Parser rhs)
{
	SerialParser* s = new SerialParser();
	s->add(Parser(true, new LiteralParser(lhs)));
	s->add(rhs);
	return Parser(true, s);
}

Parser operator >> (Parser lhs, ForceApplication rhs)
{
	SerialParser* s = lhs.as<SerialParser>();
	if (s != NULL)
	{
		s->add(rhs);
		return lhs;
	}
	else
	{
		s = new SerialParser();
		s->add(lhs);
		s->add(rhs);
		return Parser(true, s);
	}
}

Parser operator >> (const char* lhs, ForceApplication rhs)
{
	SerialParser* s = new SerialParser();
	s->add(Parser(true, new LiteralParser(lhs)));
	s->add(rhs);
	return Parser(true, s);
}


////////////////////////////////////////////////////////////


RepetitionParser::RepetitionParser(RepetitionParser const& other)
: m_parser(other.m_parser)
, m_type(other.m_type)
{ }

RepetitionParser::~RepetitionParser()
{ }


TokenIter RepetitionParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	if (m_type == rptOptional)
	{
		iter = m_parser.parse(iter, tree, result);
		if (result.status() == failure) result.setSuccess();
		return iter;
	}
	else if (m_type == rptZeroOrMore)
	{
		Node node(iter);
		while (true)
		{
			TokenIter newiter = m_parser.parse(iter, &node, result);
			if (result.status() == fatal)
			{
				return iter;
			}
			else if (result.status() == success)
			{
				iter = newiter;
			}
			else break;
		}
		tree->merge(&node);
		result.setSuccess();
		return iter;
	}
	else if (m_type == rptOneOrMore)
	{
		Node node(iter);
		iter = m_parser.parse(iter, &node, result);
		if (result.status() != success) return iter;
		while (true)
		{
			TokenIter newiter = m_parser.parse(iter, &node, result);
			if (result.status() == fatal)
			{
				return iter;
			}
			else if (result.status() == success)
			{
				iter = newiter;
			}
			else break;
		}
		tree->merge(&node);
		result.setSuccess();
		return iter;
	}

	throw runtime_error("[RepetitionParser::parse] invalid repetition type");
}

Parser operator - (Parser parser)
{
	return Parser(true, new RepetitionParser(parser, rptOptional));
}
Parser operator * (Parser parser)
{
	return Parser(true, new RepetitionParser(parser, rptZeroOrMore));
}
Parser operator + (Parser parser)
{
	return Parser(true, new RepetitionParser(parser, rptOneOrMore));
}


////////////////////////////////////////////////////////////


AlternativeParser::AlternativeParser()
{ }

AlternativeParser::AlternativeParser(AlternativeParser const& other)
: m_sub(other.m_sub)
{ }

AlternativeParser::~AlternativeParser()
{ }


TokenIter AlternativeParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	list<Parser>::const_iterator it = m_sub.begin();
	list<Parser>::const_iterator e = m_sub.end();
	for (; it != e; ++it)
	{
		iter = (*it).parse(iter, tree, result);
		if (result.status() == fatal) return iter;
		if (result.status() == success) return iter;
	}

	result.setFailure(iter, "syntax error");
	return iter;
}


Parser operator | (Parser lhs, Parser rhs)
{
	AlternativeParser* a = lhs.as<AlternativeParser>();
	if (a != NULL)
	{
		a->add(rhs);
		return lhs;
	}
	else
	{
		a = new AlternativeParser();
		a->add(lhs);
		a->add(rhs);
		return Parser(true, a);
	}
}

Parser operator | (Parser lhs, const char* rhs)
{
	AlternativeParser* a = lhs.as<AlternativeParser>();
	if (a != NULL)
	{
		a->add(Parser(true, new LiteralParser(rhs)));
		return lhs;
	}
	else
	{
		a = new AlternativeParser();
		a->add(lhs);
		a->add(Parser(true, new LiteralParser(rhs)));
		return Parser(true, a);
	}
}

Parser operator | (const char* lhs, Parser rhs)
{
	AlternativeParser* a = new AlternativeParser();
	a->add(Parser(true, new LiteralParser(lhs)));
	a->add(rhs);
	return Parser(true, a);
}


////////////////////////////////////////////////////////////


ListParser::ListParser(Parser content, Parser delimiter, bool emptyAllowed)
: m_content(content)
, m_delimiter(delimiter)
, m_emptyAllowed(emptyAllowed)
{ }

ListParser::ListParser(ListParser const& other)
: m_content(other.m_content)
, m_delimiter(other.m_delimiter)
, m_emptyAllowed(other.m_emptyAllowed)
{ }

ListParser::~ListParser()
{ }


TokenIter ListParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	// first entry
	{
		Node node(iter);
		iter = m_content.parse(iter, &node, result);
		if (result.status() == fatal) return iter;
		else if (result.status() == failure)
		{
			if (m_emptyAllowed) result.setSuccess();
			return iter;
		}
		tree->merge(&node);
	}

	// all further entries
	while (true)
	{
		Node node1(iter);
		iter = m_delimiter.parse(iter, &node1, result);
		if (result.status() == fatal) return iter;
		else if (result.status() == failure)
		{
			result.setSuccess();
			return iter;
		}
		tree->merge(&node1);

		Node node2(iter);
		iter = m_content.parse(iter, &node2, result);
		if (result.status() != success) return iter;
		tree->merge(&node2);
	}
}


Parser operator % (Parser lhs, Parser rhs)
{
	return Parser(true, new ListParser(lhs, rhs, true));
}

Parser operator % (Parser lhs, const char* rhs)
{
	return Parser(true, new ListParser(lhs, Parser(true, new LiteralParser(rhs)), true));
}

Parser operator / (Parser lhs, Parser rhs)
{
	return Parser(true, new ListParser(lhs, rhs, false));
}

Parser operator / (Parser lhs, const char* rhs)
{
	return Parser(true, new ListParser(lhs, Parser(true, new LiteralParser(rhs)), false));
}


////////////////////////////////////////////////////////////


DifferenceParser::DifferenceParser(DifferenceParser const& other)
: m_good(other.m_good)
, m_bad(other.m_bad)
{ }

DifferenceParser::~DifferenceParser()
{ }


TokenIter DifferenceParser::parse(TokenIter iter, Node* tree, ParseResult& result) const
{
	Node tmp(iter);
	m_bad.parse(iter, &tmp, result);
	if (result.status() != failure)
	{
		result.setFailure(iter, "syntax error");
		return iter;
	}

	iter = m_good.parse(iter, tree, result);
	return iter;
}

Parser operator - (Parser lhs, Parser rhs)
{
	return Parser(true, new DifferenceParser(lhs, rhs));
}
Parser operator - (Parser lhs, const char* rhs)
{
	return Parser(true, new DifferenceParser(lhs, Parser(true, new LiteralParser(rhs))));
}


////////////////////////////////////////////////////////////


// static
string ErrorMessage::format(const Token* token, string message)
{
	stringstream ss;
	ss << "error in line " << token->line();
	ss << ": " << message;
	string value = token->value();
	if (! value.empty()) ss << " near '" << value << "'";
	return ss.str();
}

// static
string ErrorMessage::format(const Node* node, string message)
{ return format(node->token(), message); }
