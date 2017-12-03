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

#include <fstream>
#include <iostream>

using nlohmann::json;
using nlohmann::json_uri;
using nlohmann::json_schema_draft4::json_validator;

static void usage(const char *name)
{
	std::cerr << "Usage: " << name << " <gltf file> <gltf schema dir>\n";
	std::cerr << "  schema dir : $glTF/specification/2.0/schema\n";
	exit(EXIT_FAILURE);
}

#if 0
	resolver r(nlohmann::json_schema_draft4::root_schema,
			   nlohmann::json_schema_draft4::root_schema["id"]);
	schema_refs_.insert(r.schema_refs.begin(), r.schema_refs.end());
	assert(r.undefined_refs.size() == 0);
#endif

#if 0
static void loader(const json_uri &uri, json &schema)
{
	std::fstream lf("." + uri.path());
	if (!lf.good())
		throw std::invalid_argument("could not open " + uri.url() + " tried with " + uri.path());

	try {
		lf >> schema;
	} catch (std::exception &e) {
		throw e;
	}
}
#endif

bool validate(const std::string &schema_dir, const std::string &filename)
{
  std::string gltf_schema = schema_dir + "/glTF.schema.json";

	std::fstream f(gltf_schema);
	if (!f.good()) {
		std::cerr << "could not open " << gltf_schema << " for reading\n";
    return false;
  }

	// 1) Read the schema for the document you want to validate
	json schema;
	try {
		f >> schema;
	} catch (std::exception &e) {
		std::cerr << e.what() << " at " << f.tellp() << " - while parsing the schema\n";
		return false;
	}

	// 2) create the validator and
	json_validator validator([&schema_dir](const json_uri &uri, json &schema) {
    std::cout << "uri.url  : " << uri.url() << std::endl;
    std::cout << "uri.path : " << uri.path() << std::endl;

    std::fstream lf(schema_dir + "/" + uri.path());
    if (!lf.good())
      throw std::invalid_argument("could not open " + uri.url() + " tried with " + uri.path());

    try {
      lf >> schema;
    } catch (std::exception &e) {
      throw e;
    }
  }, [](const std::string &, const std::string &) {});

	try {
		// insert this schema as the root to the validator
		// this resolves remote-schemas, sub-schemas and references via the given loader-function
		validator.set_root_schema(schema);
	} catch (std::exception &e) {
		std::cerr << "setting root schema failed\n";
		std::cerr << e.what() << "\n";
	}

	// 3) do the actual validation of the document
	json document;

	std::fstream d(filename);
	if (!d.good()) {
		std::cerr << "could not open " << filename << " for reading\n";
    return false;
  }

	try {
		d >> document;
		validator.validate(document);
	} catch (std::exception &e) {
		std::cerr << "schema validation failed\n";
		std::cerr << e.what() << " at offset: " << d.tellg() << "\n";
		return false;
	}

	std::cerr << "document is valid\n";

  return true;

}

int main(int argc, char *argv[])
{
	if (argc != 3)
		usage(argv[0]);

  bool ret = validate(argv[1], argv[2]);

	return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
