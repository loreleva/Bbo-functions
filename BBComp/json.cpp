
#include "json.h"
#include <fstream>


// static
Json Json::m_undefined;


// #define INDENT_CHAR ' '
#define INDENT_CHAR '\t'


Json::Json()
: m_ptr(std::make_shared<Data>())
{ }

Json::Json(bool value)
: m_ptr(std::make_shared<Data>(value))
{ }

Json::Json(double value)
: m_ptr(std::make_shared<Data>(value))
{ }

Json::Json(const char* value)
: m_ptr(std::make_shared<Data>(std::string(value)))
{ }

Json::Json(std::string value)
: m_ptr(std::make_shared<Data>(value))
{ }

Json::Json(std::vector<bool> const& arr)
: m_ptr(std::make_shared<Data>(Array(arr.size(), Json(false))))
{
	for (std::size_t i=0; i<arr.size(); i++) m_ptr->as<Array>()[i] = Json(arr[i]);
}

Json::Json(std::vector<double> const& arr)
: m_ptr(std::make_shared<Data>(Array(arr.size(), Json(0.0))))
{
	for (std::size_t i=0; i<arr.size(); i++) m_ptr->as<Array>()[i] = Json(arr[i]);
}

Json::Json(std::vector<std::string> const& arr)
: m_ptr(std::make_shared<Data>(Array(arr.size(), Json(std::string()))))
{
	for (std::size_t i=0; i<arr.size(); i++) m_ptr->as<Array>()[i] = Json(arr[i]);
}

Json::Json(std::map<std::string, bool> const& obj)
: m_ptr(std::make_shared<Data>(Object()))
{
	for (std::map<std::string, bool>::const_iterator it=obj.begin(); it != obj.end(); ++it)
	{
		m_ptr->as<Object>()[it->first] = Json(it->second);
	}
}

Json::Json(std::map<std::string, double> const& obj)
: m_ptr(std::make_shared<Data>(Object()))
{
	for (std::map<std::string, double>::const_iterator it=obj.begin(); it != obj.end(); ++it)
	{
		m_ptr->as<Object>()[it->first] = Json(it->second);
	}
}

Json::Json(std::map<std::string, std::string> const& obj)
: m_ptr(std::make_shared<Data>(Object()))
{
	for (std::map<std::string, std::string>::const_iterator it=obj.begin(); it != obj.end(); ++it)
	{
		m_ptr->as<Object>()[it->first] = Json(it->second);
	}
}

Json::Json(Json const& other)
: m_ptr(other.m_ptr)
{ }

Json::Json(ConstructionTypename tn)
: m_ptr((tn == json_null) ? (std::make_shared<Data>(Null())) : ( (tn == json_object) ? (std::make_shared<Data>(Object())) : (std::make_shared<Data>(Array())) ))
{ }

Json::Json(ConstructionParse p, std::string s)
{ parse(s); }

Json::Json(std::istream& str)
{
	str.unsetf(std::ios::skipws);
	std::istream_iterator<char> iter(str), end;
	parseJson(iter, end, iter);
}


std::vector<bool> Json::asBooleanArray()
{
	assertArray();
	std::size_t sz = size();
	std::vector<bool> ret(sz);
	for (std::size_t i=0; i<sz; i++) ret[i] = data().as<Array>()[i].asBoolean();
	return ret;
}

std::vector<double> Json::asNumberArray()
{
	assertArray();
	std::size_t sz = size();
	std::vector<double> ret(sz);
	for (std::size_t i=0; i<sz; i++) ret[i] = data().as<Array>()[i].asNumber();
	return ret;
}

std::vector<std::string> Json::asStringArray()
{
	assertArray();
	std::size_t sz = size();
	std::vector<std::string> ret(sz);
	for (std::size_t i=0; i<sz; i++) ret[i] = data().as<Array>()[i].asString();
	return ret;
}

std::map<std::string, bool> Json::asBooleanObject()
{
	assertObject();
	std::map<std::string, bool> ret;
	for (const_object_iterator it=object_begin(); it != object_end(); ++it)
	{
		ret[it->first] = it->second.asBoolean();
	}
	return ret;
}

std::map<std::string, double> Json::asNumberObject()
{
	assertObject();
	std::map<std::string, double> ret;
	for (const_object_iterator it=object_begin(); it != object_end(); ++it)
	{
		ret[it->first] = it->second.asNumber();
	}
	return ret;
}

std::map<std::string, std::string> Json::asStringObject()
{
	assertObject();
	std::map<std::string, std::string> ret;
	for (const_object_iterator it=object_begin(); it != object_end(); ++it)
	{
		ret[it->first] = it->second.asString();
	}
	return ret;
}

bool Json::operator == (Json const& other) const
{
	if (data().type() == type_undefined) return false;     // never equal
	if (data().type() != other.data().type()) return false;
	if (m_ptr == other.m_ptr) return true;

	switch (data().type())
	{
	case type_null:
		return true;
	case type_boolean:
		return (data().as<bool>() == other.data().as<bool>());
	case type_number:
		return (data().as<double>() == other.data().as<double>());
	case type_string:
		return (data().as<std::string>() == other.data().as<std::string>());
	case type_object:
	{
		if (data().as<Object>().size() != other.data().as<Object>().size()) return false;
		const_object_iterator i1 = object_begin();
		const_object_iterator i2 = other.object_begin();
		for (; i1 != object_end(); ++i1, ++i2) if (*i1 != *i2) return false;
		return true;
	}
	case type_array:
	{
		if (data().as<Array>().size() != other.data().as<Array>().size()) return false;
		const_array_iterator i1 = array_begin();
		const_array_iterator i2 = other.array_begin();
		for (; i1 != array_end(); ++i1, ++i2) if (*i1 != *i2) return false;
		return true;
	}
	default:
		throw std::runtime_error("json internal error");
	}
}

Json Json::clone() const
{
	if (isUndefined()) return Json();
	else if (isNull()) { return Json(json_null); }
	else if (isBoolean()) { return Json((bool)(*this)); }
	else if (isNumber()) { return Json((double)(*this)); }
	else if (isString()) { return Json((std::string)(*this)); }
	else if (isObject())
	{
		Json ret(json_object);
		for (const_object_iterator it = object_begin(); it != object_end(); ++it)
		{
			if (it->second.isValid()) ret[it->first] = it->second.clone();
		}
		return ret;
	}
	else if (isArray())
	{
		Json ret(json_array);
		for (const_array_iterator it = array_begin(); it != array_end(); ++it) ret.push_back(it->clone());
		return ret;
	}
	else throw std::runtime_error("json internal error");
}

// static
void Json::outputString(std::ostream& str, std::string const& s)
{
	str << "\"";
	for (std::size_t i=0; i<s.size(); i++)
	{
		char c = s[i];
		if (c >= 0 && c < 32)
		{
			if (c == '\b') str << "\\b";
			else if (c == '\f') str << "\\f";
			else if (c == '\n') str << "\\n";
			else if (c == '\r') str << "\\r";
			else if (c == '\t') str << "\\t";
			else str << "\\u00" << ((c < 16) ? "0" : "1") << (((c & 15) < 10) ? (char)((c & 15) + '0') : (char)((c & 15) + 'a' - 10) );
		}
		else if (c == '\"') str << "\\\"";
		else if (c == '\\') str << "\\\\";
		else str << c;
	}
	str << "\"";
}

void Json::outputJson(std::ostream& str, int depth) const
{
	if (isUndefined()) str << "undefined";
	else if (isNull()) str << "null";
	else if (isBoolean())
	{
		if (asBoolean()) str << "true";
		else str << "false";
	}
	else if (isNumber())
	{
		double d = asNumber();
		if (d > 1e308) str << "1e308";
		else if (d < -1e308) str << "-1e308";
		else
		{
			std::streamsize oldp = str.precision(15);
			str << d;
			str.precision(oldp);
		}
	}
	else if (isString()) outputString(str, asString());
	else if (isObject())
	{
		str << "{";
		const_object_iterator it = object_begin();
		const_object_iterator end = object_end();
		bool first = true;
		for (; it != end; ++it)
		{
			if (it->second.isValid())
			{
				if (! first) str << ","; else first = false;
				if (depth >= 0) str << "\n" << std::string(depth + 1, INDENT_CHAR);
				outputString(str, it->first);
				str << ":";
				it->second.outputJson(str, depth < 0 ? depth : depth + 1);
			}
		}
		if (! first && depth >= 0) str << "\n" << std::string(depth, INDENT_CHAR);
		str << "}";
	}
	else if (isArray())
	{
		str << "[";
		const_array_iterator it = array_begin();
		const_array_iterator end = array_end();
		if (it != end)
		{
			if (depth >= 0) str << "\n" << std::string(depth + 1, INDENT_CHAR);
			it->outputJson(str, depth < 0 ? depth : depth + 1);
			++it;
			for (; it != end; ++it)
			{
				str << ",";
				if (depth >= 0) str << "\n" << std::string(depth + 1, INDENT_CHAR);
				it->outputJson(str, depth < 0 ? depth : depth + 1);
			}
		}
		if (depth >= 0) str << "\n" << std::string(depth, INDENT_CHAR);
		str << "]";
	}
	else throw std::runtime_error("json internal error");
}

bool Json::load(std::string filename)
{
	std::ifstream ifs(filename.c_str());
	if (! ifs.is_open()) return false;
	try
	{
		ifs >> *this;
		return (ifs.good());
	}
	catch (...)
	{
		return false;
	}
}

bool Json::save(std::string filename, bool humanreadable) const
{
	std::ofstream ofs(filename.c_str());
	if (! ofs.is_open()) return false;
	try
	{
		outputJson(ofs, humanreadable ? 0 : -1);
		if (humanreadable) ofs << std::endl;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
