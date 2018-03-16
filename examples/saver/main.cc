#include <fstream>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"

int main(int argc, char *argv[])
{
	if (argc != 3) {
    std::cout << "Needs input.gltf output.gltf" << std::endl;
    return EXIT_FAILURE;
  }

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string input_filename(argv[1]);
  std::string output_filename(argv[2]);

  // assume ascii glTF.
  bool ret = loader.LoadASCIIFromFile(&model, &err, input_filename.c_str());
  if (!ret) {
    if (!err.empty()) {
      std::cerr << err << std::endl;
    }
    return EXIT_FAILURE;
  }
  loader.WriteGltfSceneToFile(&model, output_filename);

  return EXIT_SUCCESS;

}
