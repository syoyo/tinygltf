cmake_minimum_required(VERSION 3.6)
project(glview)

find_package ( GLEW REQUIRED )
find_package ( OpenGL REQUIRED )

find_library(COCOA_LIBRARY Cocoa)
find_library(COREVIDEO_LIBRARY CoreVideo)
find_library(IOKIT_LIBRARY IOKit)

set(CMAKE_CXX_STANDARD 11)

include_directories(
  ../../
  ../common
  # ${OPENGL_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIR}
  ${GLFW_INCLUDE_DIR}
  )

add_executable(glview
  glview.cc
  ../common/trackball.cc
  )

target_link_libraries ( glview
  ${GLEW_LIBRARY}
  ${GLFW_LIBRARY}
  ${OPENGL_gl_LIBRARY}
  ${OPENGL_glu_LIBRARY}
  ${COCOA_LIBRARY}
  ${COREVIDEO_LIBRARY}
  ${IOKIT_LIBRARY}
  )

install ( TARGETS
  glview
  DESTINATION
  bin
  )
