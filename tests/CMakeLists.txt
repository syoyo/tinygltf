cmake_minimum_required(VERSION 3.6)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags similar to the Makefile
set(EXTRA_CXXFLAGS "")
# Uncomment the line below for strict compilation check (will work on clang 3.8+)
# set(EXTRA_CXXFLAGS "-fsanitize=address -Wall -Werror -Weverything -Wno-c++11-long-long -DTINYGLTF_APPLY_CLANG_WEVERYTHING")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/..)

# Create the normal tester executable
add_executable(tester
    tester.cc
)

# Set compiler flags for the normal version
target_compile_options(tester PRIVATE
    ${EXTRA_CXXFLAGS}
    -std=c++11
    -g
    -O0
)

# Create the noexcept tester executable
add_executable(tester_noexcept
    tester.cc
)

# Set compiler flags for the noexcept version
target_compile_options(tester_noexcept PRIVATE
    ${EXTRA_CXXFLAGS}
    -std=c++11
    -g
    -O0
)

# Add the TINYGLTF_NOEXCEPTION define for the noexcept version
target_compile_definitions(tester_noexcept PRIVATE
    TINYGLTF_NOEXCEPTION
)

# Set output directory for executables
set_target_properties(tester tester_noexcept PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
) 