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
#include <json-schema.hpp>

#include <set>

using nlohmann::json;
using nlohmann::json_uri;

#ifdef JSON_SCHEMA_BOOST_REGEX
 #include <boost/regex.hpp>
 #define REGEX_NAMESPACE boost
#elif defined(JSON_SCHEMA_NO_REGEX)
 #define NO_STD_REGEX
#else
 #include <regex>
 #define REGEX_NAMESPACE std
#endif

namespace
{

class resolver
{
	void resolve(json &schema, json_uri id)
	{
		// look for the id-field in this schema
		auto fid = schema.find("id");

		// found?
		if (fid != schema.end() &&
		    fid.value().type() == json::value_t::string)
			id = id.derive(fid.value()); // resolve to a full id with URL + path based on the parent

		// already existing - error
		if (schema_refs.find(id) != schema_refs.end())
			throw std::invalid_argument("schema " + id.to_string() + " already present in local resolver");

		// store a raw pointer to this (sub-)schema referenced by its absolute json_uri
		// this (sub-)schema is part of a schema stored inside schema_store_ so we can the a raw-pointer-ref
		schema_refs[id] = &schema;

		for (auto i = schema.begin(), end = schema.end(); i != end; ++i) {
			// FIXME: this inhibits the user adding properties with the key "default"
			if (i.key() == "default") /* default value can be objects, but are not schemas */
				continue;

			switch (i.value().type()) {

			case json::value_t::object: // child is object, it is a schema
				resolve(i.value(), id.append(json_uri::escape(i.key())));
				break;

			case json::value_t::array: {
				std::size_t index = 0;
				auto child_id = id.append(json_uri::escape(i.key()));
				for (auto &v : i.value()) {
					if (v.type() == json::value_t::object) // array element is object
						resolve(v, child_id.append(std::to_string(index)));
					index++;
				}
			} break;

			case json::value_t::string:
				if (i.key() == "$ref") {
					json_uri ref = id.derive(i.value());
					i.value() = ref.to_string();
					refs.insert(ref);
				}
				break;

			default:
				break;
			}
		}
	}

	std::set<json_uri> refs;

public:
	std::set<json_uri> undefined_refs;

	std::map<json_uri, const json *> schema_refs;

	resolver(json &schema, json_uri id)
	{
		// if schema has an id use it as name and to retrieve the namespace (URL)
		auto fid = schema.find("id");
		if (fid != schema.end())
			id = id.derive(fid.value());

		resolve(schema, id);

		// refs now contains all references
		//
		// local references should be resolvable inside the same URL
		//
		// undefined_refs will only contain external references
		for (auto r : refs) {
			if (schema_refs.find(r) == schema_refs.end()) {
				if (r.url() == id.url()) // same url means referencing a sub-schema
					                       // of the same document, which has not been found
					throw std::invalid_argument("sub-schema " + r.pointer().to_string() +
					                            " in schema " + id.to_string() + " not found");
				undefined_refs.insert(r.url());
			}
		}
	}
};

void validate_type(const json &schema, const std::string &expected_type, const std::string &name)
{
	const auto &type_it = schema.find("type");
	if (type_it == schema.end())
		/* TODO something needs to be done here, I think */
		return;

	const auto &type_instance = type_it.value();

	// any of the types in this array
	if (type_instance.type() == json::value_t::array) {
		if ((std::find(type_instance.begin(),
		               type_instance.end(),
		               expected_type) != type_instance.end()) ||
		    (expected_type == "integer" &&
		     std::find(type_instance.begin(),
		               type_instance.end(),
		               "number") != type_instance.end()))
			return;

		std::ostringstream s;
		s << expected_type << " is not any of " << type_instance << " for " << name;
		throw std::invalid_argument(s.str());

	} else { // type_instance is a string
		if (type_instance == expected_type ||
		    (type_instance == "number" && expected_type == "integer"))
			return;

		throw std::invalid_argument(name + " is " + expected_type +
		                            ", but required type is " + type_instance.get<std::string>());
	}
}

void validate_enum(const json &instance, const json &schema, const std::string &name)
{
	const auto &enum_value = schema.find("enum");
	if (enum_value == schema.end())
		return;

	if (std::find(enum_value.value().begin(), enum_value.value().end(), instance) != enum_value.value().end())
		return;

	std::ostringstream s;
	s << "invalid enum-value '" << instance << "' "
	  << "for instance '" << name << "'. Candidates are " << enum_value.value() << ".";

	throw std::invalid_argument(s.str());
}

void validate_boolean(const json & /*instance*/, const json &schema, const std::string &name)
{
	validate_type(schema, "boolean", name);
}

void validate_numeric(const json &schema, const std::string &name, double value)
{
	// multipleOf - if the rest of the division is 0 -> OK
	const auto &multipleOf = schema.find("multipleOf");
	if (multipleOf != schema.end()) {
		if (multipleOf.value().get<double>() != 0.0) {

			double v = value;
			v /= multipleOf.value().get<double>();

			if (v != (double) (long) v)
				throw std::out_of_range(name + " is not a multiple ...");
		}
	}

	const auto &maximum = schema.find("maximum");
	if (maximum != schema.end()) {
		double maxi = maximum.value();
		auto ex = std::out_of_range(name + " exceeds maximum of " + std::to_string(maxi));
		if (schema.find("exclusiveMaximum") != schema.end()) {
			if (value >= maxi)
				throw ex;
		} else {
			if (value > maxi)
				throw ex;
		}
	}

	const auto &minimum = schema.find("minimum");
	if (minimum != schema.end()) {
		double mini = minimum.value();
		auto ex = std::out_of_range(name + " exceeds minimum of " + std::to_string(mini));
		if (schema.find("exclusiveMinimum") != schema.end()) {
			if (value <= mini)
				throw ex;
		} else {
			if (value < mini)
				throw ex;
		}
	}
}

void validate_integer(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "integer", name);
	validate_numeric(schema, name, instance.get<int>());
}

void validate_unsigned(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "integer", name);
	validate_numeric(schema, name, instance.get<unsigned>());
}

void validate_float(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "number", name);
	validate_numeric(schema, name, instance.get<double>());
}

void validate_null(const json & /*instance*/, const json &schema, const std::string &name)
{
	validate_type(schema, "null", name);
}

} // anonymous namespace

namespace nlohmann
{
namespace json_schema_draft4
{

void json_validator::insert_schema(const json &input, const json_uri &id)
{
	// allocate create a copy for later storage - if resolving reference works
	std::shared_ptr<json> schema = std::make_shared<json>(input);

	do {
		// resolve all local schemas and references
		resolver r(*schema, id);

		// check whether all undefined schema references can be resolved with existing ones
		std::set<json_uri> undefined;
		for (auto &ref : r.undefined_refs)
			if (schema_refs_.find(ref) == schema_refs_.end()) // exact schema reference not found
				undefined.insert(ref);

		if (undefined.size() == 0) { // no undefined references
			// now insert all schema-references
			// check whether all schema-references are new
			for (auto &sref : r.schema_refs) {
				if (schema_refs_.find(sref.first) != schema_refs_.end())
          // HACK(syoyo): Skip duplicated schema.
          break;
					//throw std::invalid_argument("schema " + sref.first.to_string() + " already present in validator.");
			}
			// no undefined references and no duplicated schema - store the schema
			schema_store_.push_back(schema);

			// and insert all references
			schema_refs_.insert(r.schema_refs.begin(), r.schema_refs.end());

			break;
		}

		if (schema_loader_ == nullptr)
			throw std::invalid_argument("schema contains undefined references to other schemas, needed schema-loader.");

		for (auto undef : undefined) {
			json ext;

			schema_loader_(undef, ext);
			insert_schema(ext, undef.url());
		}
	} while (1);

	// store the document root-schema
	if (id == json_uri("#"))
		root_schema_ = schema;
}

void json_validator::validate(const json &instance)
{
	if (root_schema_ == nullptr)
		throw std::invalid_argument("no root-schema has been inserted. Cannot validate an instance without it.");

	validate(instance, *root_schema_, "root");
}

void json_validator::set_root_schema(const json &schema)
{
	insert_schema(schema, json_uri("#"));
}

void json_validator::validate(const json &instance, const json &schema_, const std::string &name)
{
	const json *schema = &schema_;

	// $ref resolution
	do {
		const auto &ref = schema->find("$ref");
		if (ref == schema->end())
			break;

		auto it = schema_refs_.find(ref.value().get<std::string>());

		if (it == schema_refs_.end())
			throw std::invalid_argument("schema reference " + ref.value().get<std::string>() + " not found. Make sure all schemas have been inserted before validation.");

		schema = it->second;
	} while (1); // loop in case of nested refs

	// not
	const auto attr = schema->find("not");
	if (attr != schema->end()) {
		bool ok;

		try {
			validate(instance, attr.value(), name);
			ok = false;
		} catch (std::exception &) {
			ok = true;
		}
		if (!ok)
			throw std::invalid_argument("schema match for " + name + " but a not-match is defined by schema.");
		return; // return here - not cannot be mixed with based-schemas?
	}

	// allOf, anyOf, oneOf
	const json *combined_schemas = nullptr;
	enum {
		none,
		allOf,
		anyOf,
		oneOf
	} combine_logic = none;

	{
		const auto &attr = schema->find("allOf");
		if (attr != schema->end()) {
			combine_logic = allOf;
			combined_schemas = &attr.value();
		}
	}
	{
		const auto &attr = schema->find("anyOf");
		if (attr != schema->end()) {
			combine_logic = anyOf;
			combined_schemas = &attr.value();
		}
	}
	{
		const auto &attr = schema->find("oneOf");
		if (attr != schema->end()) {
			combine_logic = oneOf;
			combined_schemas = &attr.value();
		}
	}

	if (combine_logic != none) {
		std::size_t count = 0;
		std::ostringstream sub_schema_err;

		for (const auto &s : *combined_schemas) {
			try {
				validate(instance, s, name);
				count++;
			} catch (std::exception &e) {
				sub_schema_err << "  one schema failed because: " << e.what() << "\n";

				if (combine_logic == allOf)
					throw std::out_of_range("At least one schema has failed for " + name + " where allOf them were requested.\n" + sub_schema_err.str());
			}
			if (combine_logic == oneOf && count > 1)
				throw std::out_of_range("More than one schema has succeeded for " + name + " where only oneOf them was requested.\n" + sub_schema_err.str());
		}
		if ((combine_logic == anyOf || combine_logic == oneOf) && count == 0)
			throw std::out_of_range("No schema has succeeded for " + name + " but anyOf/oneOf them should have worked.\n" + sub_schema_err.str());
	}

	// check (base) schema
	validate_enum(instance, *schema, name);

	switch (instance.type()) {
	case json::value_t::object:
		validate_object(instance, *schema, name);
		break;

	case json::value_t::array:
		validate_array(instance, *schema, name);
		break;

	case json::value_t::string:
		validate_string(instance, *schema, name);
		break;

	case json::value_t::number_unsigned:
		validate_unsigned(instance, *schema, name);
		break;

	case json::value_t::number_integer:
		validate_integer(instance, *schema, name);
		break;

	case json::value_t::number_float:
		validate_float(instance, *schema, name);
		break;

	case json::value_t::boolean:
		validate_boolean(instance, *schema, name);
		break;

	case json::value_t::null:
		validate_null(instance, *schema, name);
		break;

	default:
		assert(0 && "unexpected instance type for validation");
		break;
	}
}

void json_validator::validate_array(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "array", name);

	// maxItems
	const auto &maxItems = schema.find("maxItems");
	if (maxItems != schema.end())
		if (instance.size() > maxItems.value().get<size_t>())
			throw std::out_of_range(name + " has too many items.");

	// minItems
	const auto &minItems = schema.find("minItems");
	if (minItems != schema.end())
		if (instance.size() < minItems.value().get<size_t>())
			throw std::out_of_range(name + " has too few items.");

	// uniqueItems
	const auto &uniqueItems = schema.find("uniqueItems");
	if (uniqueItems != schema.end())
		if (uniqueItems.value().get<bool>() == true) {
			std::set<json> array_to_set;
			for (auto v : instance) {
				auto ret = array_to_set.insert(v);
				if (ret.second == false)
					throw std::out_of_range(name + " should have only unique items.");
			}
		}

	// items and additionalItems
	// default to empty schemas
	auto items_iter = schema.find("items");
	json items = {};
	if (items_iter != schema.end())
		items = items_iter.value();

	auto additionalItems_iter = schema.find("additionalItems");
	json additionalItems = {};
	if (additionalItems_iter != schema.end())
		additionalItems = additionalItems_iter.value();

	size_t i = 0;
	bool validation_done = false;

	for (auto &value : instance) {
		std::string sub_name = name + "[" + std::to_string(i) + "]";

		switch (items.type()) {

		case json::value_t::array:

			if (i < items.size())
				validate(value, items[i], sub_name);
			else {
				switch (additionalItems.type()) { // items is an array
				                                  // we need to take into consideration additionalItems
				case json::value_t::object:
					validate(value, additionalItems, sub_name);
					break;

				case json::value_t::boolean:
					if (additionalItems.get<bool>() == false)
						throw std::out_of_range("additional values in array are not allowed for " + sub_name);
					else
						validation_done = true;
					break;

				default:
					break;
				}
			}

			break;

		case json::value_t::object: // items is a schema
			validate(value, items, sub_name);
			break;

		default:
			break;
		}
		if (validation_done)
			break;

		i++;
	}
}

void json_validator::validate_object(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "object", name);

	json properties = {};
	if (schema.find("properties") != schema.end())
		properties = schema["properties"];

#if 0
	// check for default values of properties
	// and insert them into this object, if they don't exists
	// works only for object properties for the moment
	if (default_value_insertion)
		for (auto it = properties.begin(); it != properties.end(); ++it) {

			const auto &default_value = it.value().find("default");
			if (default_value == it.value().end())
				continue; /* no default value -> continue */

			if (instance.find(it.key()) != instance.end())
				continue; /* value is present */

			/* create element from default value */
			instance[it.key()] = default_value.value();
		}
#endif
	// maxProperties
	const auto &maxProperties = schema.find("maxProperties");
	if (maxProperties != schema.end())
		if (instance.size() > maxProperties.value().get<size_t>())
			throw std::out_of_range(name + " has too many properties.");

	// minProperties
	const auto &minProperties = schema.find("minProperties");
	if (minProperties != schema.end())
		if (instance.size() < minProperties.value().get<size_t>())
			throw std::out_of_range(name + " has too few properties.");

	// additionalProperties
	enum {
		True,
		False,
		Object
	} additionalProperties = True;

	const auto &additionalPropertiesVal = schema.find("additionalProperties");
	if (additionalPropertiesVal != schema.end()) {
		if (additionalPropertiesVal.value().type() == json::value_t::boolean)
			additionalProperties = additionalPropertiesVal.value().get<bool>() == true ? True : False;
		else
			additionalProperties = Object;
	}

	// patternProperties
	json patternProperties = {};
	if (schema.find("patternProperties") != schema.end())
		patternProperties = schema["patternProperties"];

	// check all elements in object
	for (auto child = instance.begin(); child != instance.end(); ++child) {
		std::string child_name = name + "." + child.key();

		bool property_or_patternProperties_has_validated = false;
		// is this a property which is described in the schema
		const auto &object_prop = properties.find(child.key());
		if (object_prop != properties.end()) {
			// validate the element with its schema
			validate(child.value(), object_prop.value(), child_name);
			property_or_patternProperties_has_validated = true;
		}

		for (auto pp = patternProperties.begin();
		     pp != patternProperties.end(); ++pp) {
#ifndef NO_STD_REGEX
			REGEX_NAMESPACE::regex re(pp.key(), REGEX_NAMESPACE::regex::ECMAScript);

			if (REGEX_NAMESPACE::regex_search(child.key(), re)) {
				validate(child.value(), pp.value(), child_name);
				property_or_patternProperties_has_validated = true;
			}
#else
			// accept everything in case of a patternProperty
			property_or_patternProperties_has_validated = true;
			break;
#endif
		}

		if (property_or_patternProperties_has_validated)
			continue;

		switch (additionalProperties) {
		case True:
			break;

		case Object:
			validate(child.value(), additionalPropertiesVal.value(), child_name);
			break;

		case False:
			throw std::invalid_argument("unknown property '" + child.key() + "' in object '" + name + "'");
			break;
		};
	}

	// required
	const auto &required = schema.find("required");
	if (required != schema.end())
		for (const auto &element : required.value()) {
			if (instance.find(element) == instance.end()) {
				throw std::invalid_argument("required element '" + element.get<std::string>() +
				                            "' not found in object '" + name + "'");
			}
		}

	// dependencies
	const auto &dependencies = schema.find("dependencies");
	if (dependencies == schema.end())
		return;

	for (auto dep = dependencies.value().cbegin();
	     dep != dependencies.value().cend();
	     ++dep) {

		// property not present in this instance - next
		if (instance.find(dep.key()) == instance.end())
			continue;

		std::string sub_name = name + ".dependency-of-" + dep.key();

		switch (dep.value().type()) {

		case json::value_t::object:
			validate(instance, dep.value(), sub_name);
			break;

		case json::value_t::array:
			for (const auto &prop : dep.value())
				if (instance.find(prop) == instance.end())
					throw std::invalid_argument("failed dependency for " + sub_name + ". Need property " + prop.get<std::string>());
			break;

		default:
			break;
		}
	}
}

static std::size_t utf8_length(const std::string &s)
{
	size_t len = 0;
	for (const unsigned char &c : s)
		if ((c & 0xc0) != 0x80)
			len++;
	return len;
}

void json_validator::validate_string(const json &instance, const json &schema, const std::string &name)
{
	validate_type(schema, "string", name);

	// minLength
	auto attr = schema.find("minLength");
	if (attr != schema.end())
		if (utf8_length(instance) < attr.value().get<size_t>()) {
			std::ostringstream s;
			s << "'" << name << "' of value '" << instance << "' is too short as per minLength ("
			  << attr.value() << ")";
			throw std::out_of_range(s.str());
		}

	// maxLength
	attr = schema.find("maxLength");
	if (attr != schema.end())
		if (utf8_length(instance) > attr.value().get<size_t>()) {
			std::ostringstream s;
			s << "'" << name << "' of value '" << instance << "' is too long as per maxLength ("
			  << attr.value() << ")";
			throw std::out_of_range(s.str());
		}

#ifndef NO_STD_REGEX
	// pattern
	attr = schema.find("pattern");
	if (attr != schema.end()) {
		REGEX_NAMESPACE::regex re(attr.value().get<std::string>(), REGEX_NAMESPACE::regex::ECMAScript);
		if (!REGEX_NAMESPACE::regex_search(instance.get<std::string>(), re))
			throw std::invalid_argument(instance.get<std::string>() + " does not match regex pattern: " + attr.value().get<std::string>() + " for " + name);
	}
#endif

	// format
	attr = schema.find("format");
	if (attr != schema.end()) {
		if (format_check_ == nullptr)
			throw std::logic_error("A format checker was not provided but a format-attribute for this string is present. " +
			                       name + " cannot be validated for " + attr.value().get<std::string>());
		format_check_(attr.value(), instance);
	}
}
}
}
