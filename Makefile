all:
	clang++ -fsanitize=address -Wall -Werror -Weverything -Wno-c++11-long-long -g -O0 -o loader_example loader_example.cc

lint:
	./cpplint.py tiny_gltf_loader.h
