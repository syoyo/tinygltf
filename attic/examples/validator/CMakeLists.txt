project(tinygltf-validator CXX)

cmake_minimum_required(VERSION 3.2)

# exe
add_executable(tinygltf-validator
    app/tinygltf-validate.cc
    src/json-schema.hpp
    src/json-schema-draft4.json.cpp
    src/json-uri.cpp
    src/json-validator.cpp)

target_include_directories(tinygltf-validator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(tinygltf-validator
    PUBLIC
        cxx_range_for) # for C++11 - flags
# Enable more compiler warnings, except when using Visual Studio compiler
if(NOT MSVC)
    target_compile_options(tinygltf-validator
        PUBLIC
            -Wall -Wextra)
endif()
target_compile_definitions(tinygltf-validator
    PRIVATE
        -DJSON_SCHEMA_VALIDATOR_EXPORTS)

# regex with boost if gcc < 4.8 - default is std::regex
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.0")
        find_package(Boost COMPONENTS regex)
        if(NOT Boost_FOUND)
            message(STATUS "GCC less then 4.9 and boost-regex NOT found - no regex used")
            target_compile_definitions(tinygltf-validator PRIVATE -DJSON_SCHEMA_NO_REGEX)
        else()
            message(STATUS "GCC less then 4.9 and boost-regex FOUND - using boost::regex")
            target_compile_definitions(tinygltf-validator PRIVATE -DJSON_SCHEMA_BOOST_REGEX)
            target_include_directories(tinygltf-validator PRIVATE ${Boost_INCLUDE_DIRS})
            target_link_libraries(tinygltf-validator PRIVATE ${Boost_LIBRARIES})
        endif()
    endif()
endif()

# test-zone
# enable_testing()

install ( TARGETS
  tinygltf-validator
  DESTINATION
  bin
  )
