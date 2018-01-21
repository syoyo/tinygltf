
# Use this for strict compilation check(will work on clang 3.8+)
#EXTRA_CXXFLAGS := -fsanitize=address -Wall -Werror -Weverything -Wno-c++11-long-long

all:
	clang++  $(EXTRA_CXXFLAGS) -std=c++11 -g -O0 -o loader_example loader_example.cc

lint:
	./cpplint.py tiny_gltf_loader.h
