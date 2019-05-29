#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "json11.hpp"

#include "valijson/adapters/json11_adapter.hpp"
#include "valijson/utils/json11_utils.hpp"

#include "valijson/schema.hpp"
#include "valijson/schema_parser.hpp"
#include "valijson/validator.hpp"

static void usage(const char *name) {
  std::cerr << "Usage: " << name << " <gltf schema dir> <gltf file>\n";
  std::cerr << "  schema dir is usually : $glTF/specification/2.0/schema\n";
  std::cerr << "    where $glTF is a directory of https://github.com/KhronosGroup/glTF\n";

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
#endif

bool validate(const std::string &schema_dir, const std::string &filename) {
  std::string gltf_schema = schema_dir + "/glTF.schema.json";

  // 1) Read the schema for the document you want to validate
  json11::Json schema_doc;
  bool ret = valijson::utils::loadDocument(gltf_schema, schema_doc);
  if (!ret) {
    std::cerr << "Failed to load schema file : " << gltf_schema << "\n";
    return false;
  }

  // 2) Parse JSON schema content using valijson
  valijson::Schema mySchema;
  valijson::SchemaParser parser;
  valijson::adapters::Json11Adapter mySchemaAdapter(schema_doc);
  parser.populateSchema(mySchemaAdapter, mySchema);

  // 3) Load a document to validate
  json11::Json target_doc;
  if (!valijson::utils::loadDocument(filename, target_doc)) {
    std::cerr << "Failed to load JSON file to validate : " << filename << "\n";
    return false;
  }

  valijson::Validator validator;
  valijson::ValidationResults results;
  valijson::adapters::Json11Adapter myTargetAdapter(target_doc);
  if (!validator.validate(mySchema, myTargetAdapter, &results)) {
    std::cerr << "Validation failed.\n";

    valijson::ValidationResults::Error error;
    unsigned int errorNum = 1;
    while (results.popError(error)) {
      std::cerr << "Error #" << errorNum << std::endl;
      std::cerr << "  ";
      for (const std::string &contextElement : error.context) {
        std::cerr << contextElement << " ";
      }
      std::cerr << std::endl;
      std::cerr << "    - " << error.description << std::endl;
      ++errorNum;
    }

    return false;
  }

  std::cout << "Valid glTF file!\n";

  return true;
}

int main(int argc, char *argv[]) {
  if (argc != 3) usage(argv[0]);

  bool ret = validate(argv[1], argv[2]);

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
