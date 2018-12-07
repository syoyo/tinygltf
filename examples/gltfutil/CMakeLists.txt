cmake_minimum_required(VERSION 3.6)
project(gltfutil)

set(CMAKE_CXX_STANDARD 11)

include_directories(../../)

file(GLOB gltfutil_sources *.cc *.h)
add_executable(gltfutil ${gltfutil_sources})

install ( TARGETS
  gltfutil
  DESTINATION
  bin
  )
