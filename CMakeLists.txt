cmake_minimum_required(VERSION 3.6)

PROJECT (tinygltf)

SET(CMAKE_CXX_STANDARD 11)

option(TINYGLTF_BUILD_LOADER_EXAMPLE "Build loader_example" ON)
option(TINYGLTF_BUILD_GL_EXAMPLES "Build GL exampels(requires glfw, OpenGL, etc)" OFF)
option(TINYGLTF_BUILD_VALIDATOR_EXAMPLE "Build validator exampe" OFF)

if (TINYGLTF_BUILD_LOADER_EXAMPLE)
  ADD_EXECUTABLE ( loader_example
    loader_example.cc
    )
endif (TINYGLTF_BUILD_LOADER_EXAMPLE)

if (TINYGLTF_BUILD_GL_EXAMPLES)
  ADD_SUBDIRECTORY ( examples/gltfutil )
  ADD_SUBDIRECTORY ( examples/glview )
endif (TINYGLTF_BUILD_GL_EXAMPLES)

if (TINYGLTF_BUILD_VALIDATOR_EXAMPLE)
  ADD_SUBDIRECTORY ( examples/validator )
endif (TINYGLTF_BUILD_VALIDATOR_EXAMPLE)
#
# TinuGLTF is a header-only library, so no library build. just install header files.
#
INSTALL ( FILES
  json.hpp
  stb_image.h
  stb_image_write.h
  tiny_gltf.h
  DESTINATION
  include
  )

INSTALL ( FILES
  cmake/TinyGLTFConfig.cmake
  DESTINATION
  cmake
  )
