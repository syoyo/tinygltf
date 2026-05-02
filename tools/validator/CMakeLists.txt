add_executable(tinygltf3_validator
  validator.cc
)

target_include_directories(tinygltf3_validator PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

if (TINYGLTF_USE_CUSTOM_JSON)
  target_compile_definitions(tinygltf3_validator PRIVATE TINYGLTF_USE_CUSTOM_JSON)
endif ()

set_target_properties(tinygltf3_validator PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  OUTPUT_NAME tinygltf3-validator
)

install(TARGETS tinygltf3_validator
  DESTINATION bin
)
