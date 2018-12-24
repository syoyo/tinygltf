cmake_minimum_required(VERSION 3.6)
project(glview)

set ( CMAKE_PREFIX_PATH cmake )

find_package ( GLEW REQUIRED )
find_package ( GLFW3 REQUIRED )
find_package ( OpenGL REQUIRED )

if (apple)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(COREVIDEO_LIBRARY CoreVideo)
  find_library(IOKIT_LIBRARY IOKit)
endif (apple)

set(CMAKE_CXX_STANDARD 11)

include_directories(
  ../../
  ../common
  # ${OPENGL_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIR}
  ${GLFW3_INCLUDE_DIR}
  )

add_executable(glview
  glview.cc
  ../common/trackball.cc
  )

target_link_libraries ( glview
  ${GLEW_LIBRARY}
  ${GLFW3_glfw_LIBRARY}
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
