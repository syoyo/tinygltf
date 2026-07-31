cmake_minimum_required(VERSION 3.10)

project(tinygltf_v3 C)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED On)
set(CMAKE_C_EXTENSIONS Off)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TINYGLTF3_BUILD_TESTS "Build and run tinygltf v3 C unit tests" ON)
option(TINYGLTF3_INSTALL "Install tinygltf v3 files during install step" ON)

set(TINYGLTF3_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

if (TINYGLTF3_BUILD_TESTS)
  enable_testing()

  function(add_tinygltf_v3_c_test target)
    add_executable(${target} ${ARGN})
    target_include_directories(${target} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    set_target_properties(${target} PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      C_EXTENSIONS OFF
    )
    add_test(NAME ${target} COMMAND ${target} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  endfunction()

  add_tinygltf_v3_c_test(tester_v3_c tests/tester_v3_c.c tiny_gltf_v3.c)
  target_compile_definitions(tester_v3_c PRIVATE TINYGLTF3_ENABLE_FS)

  add_tinygltf_v3_c_test(tester_v3_c_v1port tests/tester_v3_c_v1port.c tiny_gltf_v3.c)
  target_compile_definitions(tester_v3_c_v1port PRIVATE TINYGLTF3_ENABLE_FS)

  add_tinygltf_v3_c_test(tester_v3_json_c tests/tester_v3_json_c.c)

  add_tinygltf_v3_c_test(tester_v3_freestanding tests/tester_v3_freestanding.c)
  if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(tester_v3_freestanding PRIVATE -ffreestanding)
  endif()
endif ()

if (TINYGLTF3_INSTALL)
  install(FILES
    tiny_gltf_v3.h
    tiny_gltf_v3.c
    tinygltf_json_c.h
    DESTINATION
    ${TINYGLTF3_INSTALL_DIR}
  )
endif ()
