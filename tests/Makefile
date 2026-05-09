# Use this for strict compilation check(will work on clang 3.8+)
#EXTRA_CXXFLAGS := -fsanitize=address -Wall -Werror -Weverything -Wno-c++11-long-long -DTINYGLTF_APPLY_CLANG_WEVERYTHING

all: ../tiny_gltf.h tester_v3_c tester_v3_c_v1port
	clang++  -I../ $(EXTRA_CXXFLAGS) -std=c++11 -g -O0 -o tester tester.cc
	clang++ -DTINYGLTF_NOEXCEPTION -I../ $(EXTRA_CXXFLAGS) -std=c++11 -g -O0 -o tester_noexcept tester.cc
	clang++ -DTINYGLTF_USE_CUSTOM_JSON -I../ $(EXTRA_CXXFLAGS) -std=c++11 -g -O0 -o tester_intensive_customjson tester_intensive_customjson.cc

tester_v3_c: tester_v3_c.c ../tiny_gltf_v3.h ../tiny_gltf_v3.c ../tinygltf_json_c.h
	clang -I../ -std=c11 -g -O0 -DTINYGLTF3_ENABLE_FS -o tester_v3_c tester_v3_c.c ../tiny_gltf_v3.c

tester_v3_c_v1port: tester_v3_c_v1port.c ../tiny_gltf_v3.h ../tiny_gltf_v3.c ../tinygltf_json_c.h
	clang -I../ -std=c11 -g -O0 -DTINYGLTF3_ENABLE_FS -o tester_v3_c_v1port tester_v3_c_v1port.c ../tiny_gltf_v3.c
