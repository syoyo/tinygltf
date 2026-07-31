#include <fstream>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

//#define TINYGLTF_NO_STB_IMAGE_WRITE

#ifndef TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif

// If using a modern Microsoft Compiler, this define supress compilation
// warnings in stb_image_write
//#define STBI_MSC_SECURE_CRT

#include "tiny_gltf.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Needs input.gltf output.gltf" << std::endl;
    return EXIT_FAILURE;
  }

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  std::string input_filename(argv[1]);
  std::string output_filename(argv[2]);
  std::string embedded_filename =
      output_filename.substr(0, output_filename.size() - 5) + "-Embedded.gltf";

  // assume ascii glTF.
  bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
  if (!warn.empty()) {
    std::cout << "warn : " << warn << std::endl;
  }
  if (!ret) {
    if (!err.empty()) {
      std::cerr << err << std::endl;
    }
    return EXIT_FAILURE;
  }
  loader.WriteGltfSceneToFile(&model, output_filename);

  // Embedd buffers and images
#ifndef TINYGLTF_NO_STB_IMAGE_WRITE
  loader.WriteGltfSceneToFile(&model, embedded_filename, true, true);
#endif

  return EXIT_SUCCESS;
}
