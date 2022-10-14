
#pragma once

//
// JSON (JavaScript Object Notation) class.
//

#include "variant.h"
#include "vector.h"

#include <stdexcept>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <istream>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <cmath>


// for convenient construction of null and empty containers
enum ConstructionTypename
{ json_null, json_object, json_array };

// for convenient construction from a string representation
enum ConstructionParse
{ json_parse };


class Json
{
protected:
	friend class std::map<std::string, Json>;  // needs access to the default constructor

	struct Undefined { };
	struct Null { };
	typedef std::map<std::string, Json> Object;
	typedef std::vector<Json> Array;

	enum Type
	{
		type_undefined = 0,
		type_null = 1,
		type_boolean = 2,
		type_number = 3,
		type_string = 4,
		type_object = 5,
		type_array = 6,
	};

	typedef Variant<Undefined, Null, bool, double, std::string, Object, Array> Data;
	typedef std::shared_ptr<Data> DataPtr;

public:
	// iterators
	typedef Object::iterator          object_iterator;
	typedef Object::const_iterator    const_object_iterator;
	typedef Array::iterator           array_iterator;
	typedef Array::const_iterator     const_array_iterator;

	// construction
	Json();   // Create an "undefined" or "invalid" json object. This is important for lazy operator [] access to object members.
	Json(bool value);
	Json(double value);
	Json(const char* value);
	Json(std::string value);
	Json(Json const& other);
	Json(ConstructionTypename tn);
	Json(ConstructionParse p, std::string s);
	Json(std::istream& str);
	Json(std::vector<bool> const& arr);
	Json(std::vector<double> const& arr);
	Json(std::vector<std::string> const& arr);
	Json(std::map<std::string, bool> const& obj);
	Json(std::map<std::string, double> const& obj);
	Json(std::map<std::string, std::string> const& obj);

	template <class ITER>
	Json(ITER begin, ITER end)
	{ parseJson(begin, end, begin); }

	// type information
	inline bool isUndefined() const
	{ return (data().type() == type_undefined); }
	inline bool isNull() const
	{ return (data().type() == type_null); }
	inline bool isBoolean() const
	{ return (data().type() == type_boolean); }
	inline bool isNumber() const
	{ return (data().type() == type_number); }
	inline bool isString() const
	{ return (data().type() == type_string); }
	inline bool isObject() const
	{ return (data().type() == type_object); }
	inline bool isArray() const
	{ return (data().type() == type_array); }

	// type asserts
	inline void assertValid() const
	{ if (! isValid()) throw std::runtime_error("Json value is undefined"); }
	inline void assertNull() const
	{ if (! isNull()) throw std::runtime_error("Json value is not null"); }
	inline void assertBoolean() const
	{ if (! isBoolean()) throw std::runtime_error("Json value is not boolean"); }
	inline void assertNumber() const
	{ if (! isNumber()) throw std::runtime_error("Json value is not a number"); }
	inline void assertString() const
	{ if (! isString()) throw std::runtime_error("Json value is not a string"); }
	inline void assertObject() const
	{ if (! isObject()) throw std::runtime_error("Json value is not an object"); }
	inline void assertArray() const
	{ if (! isArray()) throw std::runtime_error("Json value is not an array"); }
	inline void assertArray(std::size_t index) const
	{
		assertArray();
#ifdef DEBUG
		if (index >= m_ptr->as<Array>().size()) throw std::runtime_error("Json array index out of bounds");
#endif
	}

	// elementary type reference access
	inline bool asBoolean() const
	{ assertBoolean(); return data().as<bool>(); }
	inline double asNumber() const
	{ assertNumber(); return data().as<double>(); }
	inline std::string asString() const
	{ assertString(); return data().as<std::string>(); }

	// elementary type read access
	inline operator bool() const
	{ return asBoolean(); }
	inline operator double() const
	{ return asNumber(); }
	inline operator std::string() const
	{ return asString(); }

	// elementary type read access with default value
	inline bool operator () (bool defaultvalue)
	{ if (isValid()) return asBoolean(); else return defaultvalue; }
	inline double operator () (double defaultvalue)
	{ if (isValid()) return asNumber(); else return defaultvalue; }
	inline std::string operator () (const char* defaultvalue)
	{ if (isValid()) return asString(); else return defaultvalue; }
	inline std::string operator () (std::string defaultvalue)
	{ if (isValid()) return asString(); else return defaultvalue; }

	// container (array or object) size
	inline std::size_t size() const
	{
		if (isObject()) return data().as<Object>().size();
		else if (isArray()) return data().as<Array>().size();
		else throw std::runtime_error("Json value is not a container (object or array)");
	}

	// object member test
	inline bool has(const char* key) const
	{ return has(std::string(key)); }
	inline bool has(std::string key) const
	{ assertObject(); return (data().as<Object>().find(key) != object_end()); }

	// container operator access
	inline Json& operator [] (const char* key)
	{ return operator [] (std::string(key)); }
	inline Json& operator [] (std::string key)
	{ assertObject(); return data().as<Object>()[key]; }
	inline Json const& operator [] (const char* key) const
	{ return operator [] (std::string(key)); }
	inline Json const& operator [] (std::string key) const
	{ assertObject(); if (! has(key)) return m_undefined; else return (const_cast<Object&>(data().as<Object>()))[key]; }
	inline Json& operator [] (std::size_t index)
	{ assertArray(index); return data().as<Array>()[index]; }
	inline Json const& operator [] (std::size_t index) const
	{ assertArray(index); return data().as<Array>()[index]; }
	inline Json& operator [] (int index)
	{ assertArray(index); return data().as<Array>()[index]; }
	inline Json const& operator [] (int index) const
	{ assertArray(index); return data().as<Array>()[index]; }

	// container iterator access
	inline object_iterator object_begin()
	{ assertObject(); return data().as<Object>().begin(); }
	inline const_object_iterator object_begin() const
	{ assertObject(); return data().as<Object>().begin(); }
	inline object_iterator object_end()
	{ assertObject(); return data().as<Object>().end(); }
	inline const_object_iterator object_end() const
	{ assertObject(); return data().as<Object>().end(); }
	inline array_iterator array_begin()
	{ assertArray(); return data().as<Array>().begin(); }
	inline const_array_iterator array_begin() const
	{ assertArray(); return data().as<Array>().begin(); }
	inline array_iterator array_end()
	{ assertArray(); return data().as<Array>().end(); }
	inline const_array_iterator array_end() const
	{ assertArray(); return data().as<Array>().end(); }

	// pure container access
	std::vector<bool> asBooleanArray();
	std::vector<double> asNumberArray();
	std::vector<std::string> asStringArray();
	std::map<std::string, bool> asBooleanObject();
	std::map<std::string, double> asNumberObject();
	std::map<std::string, std::string> asStringObject();

	// comparison
	bool operator == (Json const& other) const;
	inline bool operator == (bool other) const
	{ return (isBoolean() && asBoolean() == other); }
	inline bool operator == (double other) const
	{ return (isNumber() && asNumber() == other); }
	inline bool operator == (const char* other) const
	{ return (isString() && asString() == other); }
	inline bool operator == (std::string other) const
	{ return (isString() && asString() == other); }
	inline bool operator != (Json const& other) const
	{ return ! (operator == (other)); }
	inline bool operator != (bool other) const
	{ return ! (operator == (other)); }
	inline bool operator != (double other) const
	{ return ! (operator == (other)); }
	inline bool operator != (const char* other) const
	{ return ! (operator == (other)); }
	inline bool operator != (std::string other) const
	{ return ! (operator == (other)); }

	// value assignment and container preparation (write access)
	inline Json& operator = (bool value)
	{ m_ptr = std::shared_ptr<Data>(new Data(value)); return *this; }
	inline Json& operator = (double value)
	{ m_ptr = std::shared_ptr<Data>(new Data(value)); return *this; }
	inline Json& operator = (const char* value)
	{ m_ptr = std::shared_ptr<Data>(new Data(std::string(value))); return *this; }
	inline Json& operator = (std::string value)
	{ m_ptr = std::shared_ptr<Data>(new Data(value)); return *this; }
	inline Json& operator = (std::vector<bool> const& arr)
	{
		m_ptr = std::shared_ptr<Data>(new Data(Array(arr.size(), Json(false))));
		for (std::size_t i=0; i<arr.size(); i++) m_ptr->as<Array>()[i] = Json(arr[i]);
		return *this;
	}
	inline Json& operator = (std::vector<double> const& arr)
	{
		m_ptr = std::shared_ptr<Data>(new Data(Array(arr.size(), Json(0.0))));
		for (std::size_t i=0; i<arr.size(); i++) m_ptr->as<Array>()[i] = Json(arr[i]);
		return *this;
	}
	inline Json& operator = (Vector const& vec)
	{
		m_ptr = std::shared_ptr<Data>(new Data(Array(vec.size(), Json(0.0))));
		for (std::size_t i=0; i<vec.size(); i++) m_ptr->as<Array>()[i] = Json(vec[i]);
		return *this;
	}
	inline Json& operator = (std::vector<std::string> const& arr)
	{
		m_ptr = std::shared_ptr<Data>(new Data(Array(arr.size(), Json(std::string()))));
		for (std::size_t i=0; i<arr.size(); i++) m_ptr->as<Array>()[i] = Json(arr[i]);
		return *this;
	}
	inline Json& operator = (std::map<std::string, bool> const& obj)
	{
		m_ptr = std::shared_ptr<Data>(new Data(Object()));
		for (std::map<std::string, bool>::const_iterator it=obj.begin(); it != obj.end(); ++it)
		{
			m_ptr->as<Object>()[it->first] = Json(it->second);
		}
		return *this;
	}
	inline Json& operator = (std::map<std::string, double> const& obj)
	{
		m_ptr = std::shared_ptr<Data>(new Data(Object()));
		for (std::map<std::string, double>::const_iterator it=obj.begin(); it != obj.end(); ++it)
		{
			m_ptr->as<Object>()[it->first] = Json(it->second);
		}
		return *this;
	}
	inline Json& operator = (std::map<std::string, std::string> const& obj)
	{
		m_ptr = std::shared_ptr<Data>(new Data(Object()));
		for (std::map<std::string, std::string>::const_iterator it=obj.begin(); it != obj.end(); ++it)
		{
			m_ptr->as<Object>()[it->first] = Json(it->second);
		}
		return *this;
	}
	inline Json& operator = (Json const& value)
	{ m_ptr = value.m_ptr; return *this; }
	inline Json& operator = (ConstructionTypename tn)
	{
		if (tn == json_null) m_ptr = std::shared_ptr<Data>(new Data(Null()));
		else if (tn == json_object) m_ptr = std::shared_ptr<Data>(new Data(Object()));
		else if (tn == json_array) m_ptr = std::shared_ptr<Data>(new Data(Array()));
		else throw std::runtime_error("json internal error");
		return *this;
	}
	inline void push_back(Json const& value)
	{ assertArray(); data().as<Array>().push_back(value); }
	inline void insert(std::size_t index, Json const& value)
	{
		assertArray();
		if (index > data().as<Array>().size()) throw std::runtime_error("Json array index out of bounds");
		data().as<Array>().insert(array_begin() + index, value);
	}
	inline void erase(std::size_t index)
	{ assertArray(index); data().as<Array>().erase(array_begin() + index); }
	template <class T> Json& operator << (T value)
	{ push_back(value); return *this; }
	inline void erase(std::string key)
	{ assertObject(); data().as<Object>().erase(key); }

	// deep copy
	Json clone() const;

	// string <-> object conversion
	inline void parse(std::string s)
	{ std::stringstream ss(s); ss >> *this; }
	inline std::string stringify() const
	{ std::stringstream ss; ss << *this; return ss.str(); }
	friend std::istream& operator >> (std::istream& is, Json& json);
	friend std::ostream& operator << (std::ostream& os, Json const& json);

	// file I/O
	bool load(std::string filename);
	bool save(std::string filename, bool humanreadable = false) const;

protected:
	inline bool isValid() const
	{ return (m_ptr->type() != type_undefined); }

#define JSON_INTERNAL_FAIL { std::stringstream ss; ss << "json parse error at position " << std::distance(begin, iter) ; throw std::runtime_error(ss.str()); }
#define JSON_INTERNAL_PEEK(c) { if (iter == end) JSON_INTERNAL_FAIL; c = *iter; }
#define JSON_INTERNAL_READ(c) { JSON_INTERNAL_PEEK(c); ++iter; }
#define JSON_INTERNAL_SKIP \
	while (true) \
	{ \
		JSON_INTERNAL_PEEK(c); \
		if (isspace(c)) ++iter; \
		else if (c == '/') \
		{ \
			++iter; \
			JSON_INTERNAL_READ(c); \
			if (c == '/') \
			{ \
				while (c != '\n') { JSON_INTERNAL_READ(c); } \
			} \
			else if (c == '*') \
			{ \
				while (true) \
				{ \
					JSON_INTERNAL_READ(c); \
					if (c == '*') \
					{ \
						JSON_INTERNAL_PEEK(c); \
						if (c == '/') { ++iter; break; } \
					} \
				} \
			} \
		} \
		else break; \
	}

	template <class ITER>
	static std::string parseString(ITER const& begin, ITER const& end, ITER& iter)
	{
		char c;
		std::string ret;
		while (true)
		{
			JSON_INTERNAL_READ(c)
			if (c == '\"') return ret;
			else if (c == '\\')
			{
				JSON_INTERNAL_READ(c)
				if (c == '\"') ret.push_back('\"');
				else if (c == '\\') ret.push_back('\\');
				else if (c == '/') ret.push_back('/');
				else if (c == 'b') ret.push_back('\b');
				else if (c == 'f') ret.push_back('\f');
				else if (c == 'n') ret.push_back('\n');
				else if (c == 'r') ret.push_back('\r');
				else if (c == 't') ret.push_back('\t');
				else if (c == 'u')
				{
					char buf[5];
					JSON_INTERNAL_READ(buf[0]);
					JSON_INTERNAL_READ(buf[1]);
					JSON_INTERNAL_READ(buf[2]);
					JSON_INTERNAL_READ(buf[3]);
					buf[4] = 0;
					unsigned int value;
					std::stringstream ss;
					ss << std::hex << buf;
					ss >> value;
					// encode as utf-8
					if (value < 0x007f) ret.push_back((char)value);
					else if (value < 0x07ff)
					{
						ret.push_back((char)(192 + (value >> 6)));
						ret.push_back((char)(128 + (value & 63)));
					}
					else
					{
						ret.push_back((char)(224 + (value >> 12)));
						ret.push_back((char)(128 + ((value >> 6) & 63)));
						ret.push_back((char)(128 + (value & 63)));
					}
				}
				else JSON_INTERNAL_FAIL
			}
			else ret.push_back(c);
		}
	}

	template <class ITER>
	void parseJson(ITER begin, ITER end, ITER& iter)
	{
		char c;
		JSON_INTERNAL_SKIP
		JSON_INTERNAL_READ(c)
		if (c == '{')
		{
			*this = json_object;
			JSON_INTERNAL_SKIP
			JSON_INTERNAL_PEEK(c)
			if (c != '}')
			{
				do
				{
					JSON_INTERNAL_SKIP
					JSON_INTERNAL_READ(c)
					if (c != '\"') JSON_INTERNAL_FAIL;
					std::string key = parseString(begin, end, iter);
					JSON_INTERNAL_SKIP
					JSON_INTERNAL_READ(c)
					if (c != ':') JSON_INTERNAL_FAIL
					Json sub;
					sub.parseJson(begin, end, iter);
					(*this)[key] = sub;
					JSON_INTERNAL_SKIP
					JSON_INTERNAL_READ(c)
				}
				while (c == ',');
				if (c != '}') JSON_INTERNAL_FAIL
			}
			else JSON_INTERNAL_READ(c);
		}
		else if (c == '[')
		{
			*this = json_array;
			JSON_INTERNAL_SKIP
			JSON_INTERNAL_PEEK(c)
			if (c != ']')
			{
				do
				{
					Json sub;
					sub.parseJson(begin, end, iter);
					push_back(sub);
					JSON_INTERNAL_SKIP
					JSON_INTERNAL_READ(c)
				}
				while (c == ',');
				if (c != ']') JSON_INTERNAL_FAIL
			}
			else JSON_INTERNAL_READ(c);
		}
		else if (c == '\"')
		{ (*this) = parseString(begin, end, iter); }
		else if (c == 'n')
		{
			JSON_INTERNAL_READ(c); if (c != 'u') JSON_INTERNAL_FAIL;
			JSON_INTERNAL_READ(c); if (c != 'l') JSON_INTERNAL_FAIL;
			JSON_INTERNAL_READ(c); if (c != 'l') JSON_INTERNAL_FAIL;
			*this = json_null;
		}
		else if (c == 't')
		{
			JSON_INTERNAL_READ(c); if (c != 'r') JSON_INTERNAL_FAIL;
			JSON_INTERNAL_READ(c); if (c != 'u') JSON_INTERNAL_FAIL;
			JSON_INTERNAL_READ(c); if (c != 'e') JSON_INTERNAL_FAIL;
			(*this) = true;
		}
		else if (c == 'f')
		{
			JSON_INTERNAL_READ(c); if (c != 'a') JSON_INTERNAL_FAIL;
			JSON_INTERNAL_READ(c); if (c != 'l') JSON_INTERNAL_FAIL;
			JSON_INTERNAL_READ(c); if (c != 's') JSON_INTERNAL_FAIL;
			JSON_INTERNAL_READ(c); if (c != 'e') JSON_INTERNAL_FAIL;
			(*this) = false;
		}
		else
		{
			// parse number
			double value = 0.0;
			bool neg = false;
			if (c == '-') { JSON_INTERNAL_READ(c); neg = true; }
			if (c == '0') { }
			else if (c >= '1' && c <= '9')
			{
				value = (c - '0');
				while (true)
				{
					JSON_INTERNAL_PEEK(c)
					if (c >= '0' && c <= '9') { JSON_INTERNAL_READ(c); value *= 10.0; value += (c - '0'); }
					else break;
				}
			}
			else JSON_INTERNAL_FAIL
			JSON_INTERNAL_PEEK(c)
			if (c == '.')
			{
				JSON_INTERNAL_READ(c)
				JSON_INTERNAL_READ(c)
				if (c < '0' || c > '9') JSON_INTERNAL_FAIL
				double p = 0.1;
				value += p * (c - '0');
				while (true)
				{
					JSON_INTERNAL_PEEK(c)
					if (c >= '0' && c <= '9') { JSON_INTERNAL_READ(c); p *= 0.1; value += p * (c - '0'); }
					else break;
				}
			}
			if (c == 'e' || c == 'E')
			{
				JSON_INTERNAL_READ(c)
				bool eneg = false;
				int e = 0;
				JSON_INTERNAL_PEEK(c)
				if (c == '+') { JSON_INTERNAL_READ(c); JSON_INTERNAL_PEEK(c); }
				if (c == '-') { JSON_INTERNAL_READ(c); JSON_INTERNAL_PEEK(c); eneg = true; }
				if (c < '0' || c > '9') JSON_INTERNAL_FAIL
				while (true)
				{
					JSON_INTERNAL_PEEK(c)
					if (c >= '0' && c <= '9') { JSON_INTERNAL_READ(c); e *= 10; e += (c - '0'); }
					else break;
				}
				if (eneg) e = -e;
				value *= std::pow(10.0, e);
			}
			if (neg) value = -value;
			(*this) = value;
		}
	}

#undef JSON_INTERNAL_FAIL
#undef JSON_INTERNAL_READ
#undef JSON_INTERNAL_PEEK
#undef JSON_INTERNAL_SKIP

	static void outputString(std::ostream& str, std::string const& s);
	void outputJson(std::ostream& str, int depth = -1) const;

	DataPtr m_ptr;
	static Json m_undefined;  // "undefined" reference object, needed by const operator []

	inline Data& data()
	{ return *m_ptr; }
	inline Data const& data() const
	{ return *m_ptr; }
};


inline std::istream& operator >> (std::istream& str, Json& json)
{
	str.unsetf(std::ios::skipws);
	std::istream_iterator<char> iter(str), end;
	json.parseJson(iter, end, iter);
	return str;
}

inline std::ostream& operator << (std::ostream& str, Json const& json)
{ json.outputJson(str); return str; }
