#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <iostream>

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cout << "gltf_audio input.gltf/.glb" << std::endl;
  }

  float scale = 1.0f;
  if (argc > 2) {
    scale = atof(argv[2]);
  }

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

#ifdef _WIN32
#ifdef _DEBUG
  std::string input_filename(argv[1] ? argv[1]
                                     : "../../../models/Cube/Cube.gltf");
#endif
#else
  std::string input_filename(argv[1] ? argv[1] : "../../models/Cube/Cube.gltf");
#endif

  std::string ext = GetFilePathExtension(input_filename);

  bool ret = false;
  if (ext.compare("glb") == 0) {
    // assume binary glTF.
    ret =
        loader.LoadBinaryFromFile(&model, &err, &warn, input_filename.c_str());
  } else {
    // assume ascii glTF.
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
  }

  if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    printf("ERR: %s\n", err.c_str());
  }
  if (!ret) {
    printf("Failed to load .glTF : %s\n", argv[1]);
    exit(-1);
  }

  return EXIT_SUCCESS;
}
