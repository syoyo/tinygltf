cmake_minimum_required(VERSION 3.6)

PROJECT (tinygltf)

SET(CMAKE_CXX_STANDARD 11)

ADD_EXECUTABLE ( loader_example
  loader_example.cc
  )

ADD_SUBDIRECTORY ( examples/gltfutil )
ADD_SUBDIRECTORY ( examples/glview )
ADD_SUBDIRECTORY ( examples/validator )

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
