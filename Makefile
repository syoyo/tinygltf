all:
	clang++ -Wall -Werror -g -O0 -o loader_test test.cc && ./loader_test face3d.gltf

beautify:
	~/local/clang+llvm-3.6.0-x86_64-apple-darwin/bin/clang-format -i tiny_gltf_loader.h
