cmake_minimum_required(VERSION 3.6)

project(tinygltf)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TINYGLTF_BUILD_LOADER_EXAMPLE "Build loader_example(load glTF and dump infos)" ON)
option(TINYGLTF_BUILD_GL_EXAMPLES "Build GL exampels(requires glfw, OpenGL, etc)" OFF)
option(TINYGLTF_BUILD_VALIDATOR_EXAMPLE "Build validator exampe" OFF)
option(TINYGLTF_BUILD_BUILDER_EXAMPLE "Build glTF builder example" OFF)
option(TINYGLTF_HEADER_ONLY "On: header-only mode. Off: create tinygltf library(No TINYGLTF_IMPLEMENTATION required in your project)" OFF)
option(TINYGLTF_INSTALL "Install tinygltf files during install step. Usually set to OFF if you include tinygltf through add_subdirectory()" ON)
option(TINYGLTF_INSTALL_VENDOR "Install vendored nlohmann/json and nothings/stb headers" ON)

if (TINYGLTF_BUILD_LOADER_EXAMPLE)
  add_executable(loader_example
    loader_example.cc
    )
endif (TINYGLTF_BUILD_LOADER_EXAMPLE)

if (TINYGLTF_BUILD_GL_EXAMPLES)
  add_subdirectory( examples/gltfutil )
  add_subdirectory( examples/glview )
endif (TINYGLTF_BUILD_GL_EXAMPLES)

if (TINYGLTF_BUILD_VALIDATOR_EXAMPLE)
  add_subdirectory( examples/validator )
endif (TINYGLTF_BUILD_VALIDATOR_EXAMPLE)

if (TINYGLTF_BUILD_BUILDER_EXAMPLE)
  add_subdirectory ( examples/build-gltf )
endif (TINYGLTF_BUILD_BUILDER_EXAMPLE)

#
# for add_subdirectory and standalone build
#
if (TINYGLTF_HEADER_ONLY)
  add_library(tinygltf INTERFACE)

  target_include_directories(tinygltf
          INTERFACE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

else (TINYGLTF_HEADER_ONLY)
  add_library(tinygltf)
  target_sources(tinygltf PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}/tiny_gltf.cc)
  target_include_directories(tinygltf
          INTERFACE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
          )
endif (TINYGLTF_HEADER_ONLY)

if (TINYGLTF_INSTALL)
  install(TARGETS tinygltf EXPORT tinygltfTargets)
  install(EXPORT tinygltfTargets NAMESPACE tinygltf:: FILE TinyGLTFTargets.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tinygltf)
  configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TinyGLTFConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/TinyGLTFConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/TinyGLTFConfig.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tinygltf)
  # Do not install .lib even if !TINYGLTF_HEADER_ONLY

  INSTALL ( FILES
    tiny_gltf.h
    ${TINYGLTF_EXTRA_SOUECES}
    DESTINATION
    include
    )

    if(TINYGLTF_INSTALL_VENDOR)
      INSTALL ( FILES
        json.hpp
        stb_image.h
        stb_image_write.h
        DESTINATION
        include
        )
    endif()

endif(TINYGLTF_INSTALL)
