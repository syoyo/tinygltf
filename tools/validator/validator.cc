#include <cstdlib>
#include <cstring>
#include <iostream>

#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#include "tiny_gltf_v3.h"

namespace {

const char *severity_name(tg3_severity severity) {
  switch (severity) {
    case TG3_SEVERITY_WARNING:
      return "warning";
    case TG3_SEVERITY_ERROR:
      return "error";
    default:
      return "info";
  }
}

void print_errors(std::ostream &os, const tg3_error_stack *errors) {
  const uint32_t count = tg3_errors_count(errors);
  for (uint32_t i = 0; i < count; ++i) {
    const tg3_error_entry *entry = tg3_errors_get(errors, i);
    if (!entry) {
      continue;
    }

    os << severity_name(entry->severity);
    if (entry->json_path && entry->json_path[0] != '\0') {
      os << " " << entry->json_path;
    }
    if (entry->message && entry->message[0] != '\0') {
      os << ": " << entry->message;
    }
    os << '\n';
  }
}

int usage(const char *name) {
  std::cerr << "Usage: " << name << " <path/to/model.gltf|model.glb>\n";
  return EXIT_FAILURE;
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    return usage(argv[0]);
  }

  const char *filename = argv[1];

  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options options;

  std::memset(&model, 0, sizeof(model));
  model.default_scene = -1;
  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&options);

  tg3_error_code rc =
      tg3_parse_file(&model, &errors, filename,
                     static_cast<uint32_t>(std::strlen(filename)), &options);

  if (tg3_errors_count(&errors) > 0) {
    print_errors(std::cerr, &errors);
  }

  if (rc != TG3_OK) {
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return EXIT_FAILURE;
  }

  tg3_error_stack_free(&errors);
  tg3_error_stack_init(&errors);

  rc = tg3_validate(&model, &errors);

  if (tg3_errors_count(&errors) > 0) {
    print_errors(rc == TG3_OK ? std::cout : std::cerr, &errors);
  }

  if (rc == TG3_OK) {
    std::cout << filename << ": valid glTF 2.0\n";
  }

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);

  return (rc == TG3_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
