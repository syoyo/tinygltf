# Build and run tinygltf v3 C unit tests.
#
# Targets:
#   make all   — build all v3 C testers
#   make run   — run all v3 C testers
#   make clean — remove test binaries

CC      ?= cc
CFLAGS  ?= -I../ -std=c11 -g -O0 -Wall -Wextra

all: tester_v3_c tester_v3_c_v1port tester_v3_json_c tester_v3_freestanding

tester_v3_c: tester_v3_c.c ../tiny_gltf_v3.h ../tiny_gltf_v3.c ../tinygltf_json_c.h
	$(CC) $(CFLAGS) -DTINYGLTF3_ENABLE_FS -o tester_v3_c tester_v3_c.c ../tiny_gltf_v3.c

tester_v3_c_v1port: tester_v3_c_v1port.c ../tiny_gltf_v3.h ../tiny_gltf_v3.c ../tinygltf_json_c.h
	$(CC) $(CFLAGS) -DTINYGLTF3_ENABLE_FS -o tester_v3_c_v1port tester_v3_c_v1port.c ../tiny_gltf_v3.c

tester_v3_json_c: tester_v3_json_c.c ../tinygltf_json_c.h
	$(CC) $(CFLAGS) -o tester_v3_json_c tester_v3_json_c.c

tester_v3_freestanding: tester_v3_freestanding.c ../tiny_gltf_v3.h ../tiny_gltf_v3.c ../tinygltf_json_c.h
	$(CC) $(CFLAGS) -ffreestanding -o tester_v3_freestanding tester_v3_freestanding.c

run: all
	./tester_v3_c
	./tester_v3_c_v1port
	./tester_v3_json_c
	./tester_v3_freestanding

clean:
	rm -f tester_v3_c tester_v3_c_v1port tester_v3_json_c tester_v3_freestanding
