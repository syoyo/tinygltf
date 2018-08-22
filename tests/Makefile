# Use this for strict compilation check(will work on clang 3.8+)
#EXTRA_CXXFLAGS := -fsanitize=address -Wall -Werror -Weverything -Wno-c++11-long-long -DTINYGLTF_APPLY_CLANG_WEVERYTHING

all: ../tiny_gltf.h
	clang++  -I../ $(EXTRA_CXXFLAGS) -std=c++11 -g -O0 -o tester tester.cc
	clang++ -DTINYGLTF_NOEXCEPTION -I../ $(EXTRA_CXXFLAGS) -std=c++11 -g -O0 -o tester_noexcept tester.cc
