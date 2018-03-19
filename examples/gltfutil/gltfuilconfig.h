#include <string>

namespace gltfutil {
enum class ui_mode { cli, interactive };

enum class cli_action { not_set, help, dump };

struct configuration {
  std::string input_path, output_dir;
  ui_mode mode;
  cli_action action = cli_action::not_set;
  bool has_output_dir;
  bool is_valid() {
    // TODO impl check
    return false;
  }
};
}  // namespace gltfutil
