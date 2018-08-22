/*
 * Modern C++ JSON schema validator
 *
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *
 * Copyright (c) 2016 Patrick Boettcher <patrick.boettcher@posteo.de>.
 *
 * Permission is hereby  granted, free of charge, to any  person obtaining a
 * copy of this software and associated  documentation files (the "Software"),
 * to deal in the Software  without restriction, including without  limitation
 * the rights to  use, copy,  modify, merge,  publish, distribute,  sublicense,
 * and/or  sell copies  of  the Software,  and  to  permit persons  to  whom
 * the Software  is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS
 * OR IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN
 * NO EVENT  SHALL THE AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY
 * CLAIM,  DAMAGES OR  OTHER LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NLOHMANN_JSON_SCHEMA_HPP__
#define NLOHMANN_JSON_SCHEMA_HPP__

#ifdef _WIN32
#    ifdef JSON_SCHEMA_VALIDATOR_EXPORTS
#        define JSON_SCHEMA_VALIDATOR_API __declspec(dllexport)
#    else
#        define JSON_SCHEMA_VALIDATOR_API __declspec(dllimport)
#    endif
#else
#    define JSON_SCHEMA_VALIDATOR_API
#endif

#include <json.hpp>

// make yourself a home - welcome to nlohmann's namespace
namespace nlohmann
{

// a class representing a JSON-pointer RFC6901
//
// examples of JSON pointers
//
//   #     - root of the current document
//   #item - refers to the object which is identified ("id") by `item`
//           in the current document
//   #/path/to/element
//         - refers to the element in /path/to from the root-document
//
//
// The json_pointer-class stores everything in a string, which might seem bizarre
// as parsing is done from a string to a string, but from_string() is also
// doing some formatting.
//
// TODO
//   ~ and %  - codec
//   needs testing and clarification regarding the '#' at the beginning

class json_pointer
{
	std::string str_;

	void from_string(const std::string &r);

public:
	json_pointer(const std::string &s = "")
	{
		from_string(s);
	}

	void append(const std::string &elem)
	{
		str_.append(elem);
	}

	const std::string &to_string() const
	{
		return str_;
	}
};

// A class representing a JSON-URI for schemas derived from
// section 8 of JSON Schema: A Media Type for Describing JSON Documents
// draft-wright-json-schema-00
//
// New URIs can be derived from it using the derive()-method.
// This is useful for resolving refs or subschema-IDs in json-schemas.
//
// This is done implement the requirements described in section 8.2.
//
class JSON_SCHEMA_VALIDATOR_API json_uri
{
	std::string urn_;

	std::string proto_;
	std::string hostname_;
	std::string path_;
	json_pointer pointer_;

protected:
	// decodes a JSON uri and replaces all or part of the currently stored values
	void from_string(const std::string &uri);

	std::tuple<std::string, std::string, std::string, std::string, std::string> tie() const
	{
		return std::tie(urn_, proto_, hostname_, path_, pointer_.to_string());
	}

public:
	json_uri(const std::string &uri)
	{
		from_string(uri);
	}

	const std::string protocol() const { return proto_; }
	const std::string hostname() const { return hostname_; }
	const std::string path() const { return path_; }
	const json_pointer pointer() const { return pointer_; }

	const std::string url() const;

	// decode and encode strings for ~ and % escape sequences
	static std::string unescape(const std::string &);
	static std::string escape(const std::string &);

	// create a new json_uri based in this one and the given uri
	// resolves relative changes (pathes or pointers) and resets part if proto or hostname changes
	json_uri derive(const std::string &uri) const
	{
		json_uri u = *this;
		u.from_string(uri);
		return u;
	}

	// append a pointer-field to the pointer-part of this uri
	json_uri append(const std::string &field) const
	{
		json_uri u = *this;
		u.pointer_.append("/" + field);
		return u;
	}

	std::string to_string() const;

	friend bool operator<(const json_uri &l, const json_uri &r)
	{
		return l.tie() < r.tie();
	}

	friend bool operator==(const json_uri &l, const json_uri &r)
	{
		return l.tie() == r.tie();
	}

	friend std::ostream &operator<<(std::ostream &os, const json_uri &u);
};

namespace json_schema_draft4
{

extern json draft4_schema_builtin;

class JSON_SCHEMA_VALIDATOR_API json_validator
{
	std::vector<std::shared_ptr<json>> schema_store_;
	std::shared_ptr<json> root_schema_;
	std::function<void(const json_uri &, json &)> schema_loader_ = nullptr;
	std::function<void(const std::string &, const std::string &)> format_check_ = nullptr;

	std::map<json_uri, const json *> schema_refs_;

	void validate(const json &instance, const json &schema_, const std::string &name);
	void validate_array(const json &instance, const json &schema_, const std::string &name);
	void validate_object(const json &instance, const json &schema_, const std::string &name);
    void validate_string(const json &instance, const json &schema, const std::string &name);

	void insert_schema(const json &input, const json_uri &id);

public:
	json_validator(std::function<void(const json_uri &, json &)> loader = nullptr,
	               std::function<void(const std::string &, const std::string &)> format = nullptr)
	    : schema_loader_(loader), format_check_(format)
	{
	}

	// insert and set a root-schema
	void set_root_schema(const json &);

	// validate a json-document based on the root-schema
	void validate(const json &instance);
};

} // json_schema_draft4
} // nlohmann

#endif /* NLOHMANN_JSON_SCHEMA_HPP__ */
