EXTRA_CXXFLAGS=-fsanitize=address

all:
	clang++ $(EXTRA_CXXFLAGS) -g -O1 -I. -Ivalijson/include tinygltf-validate.cc json11.cpp -o tinygltf-validate
