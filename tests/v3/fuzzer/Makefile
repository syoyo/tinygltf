# tests/v3/fuzzer/Makefile — Build libFuzzer harness for tinygltf v3
#
# Requires: clang++ with libFuzzer support
#
# Targets:
#   make             — build fuzzer with ASan + UBSan
#   make run         — run fuzzer with default settings
#   make seed        — generate seed corpus from test models
#   make clean       — remove binaries and corpus

CXX       = clang++
CXXFLAGS  = -g -O1 -std=c++17 -fno-rtti -fno-exceptions
SANITIZE  = -fsanitize=fuzzer,address,undefined
INCLUDES  = -I../../..

FUZZER    = fuzz_gltf_v3
CORPUS    = corpus
ARTIFACTS = artifacts

# Fuzzer runtime options
MAX_LEN   ?= 65536
JOBS      ?= $(shell nproc 2>/dev/null || echo 4)
MAX_TIME  ?= 0

.PHONY: all run seed clean

all: $(FUZZER)

$(FUZZER): fuzz_gltf_v3.cc ../../../tiny_gltf_v3.h ../../../tinygltf_json.h
	$(CXX) $(CXXFLAGS) $(SANITIZE) $(INCLUDES) -o $@ $<

run: $(FUZZER) | $(CORPUS) $(ARTIFACTS)
	./$(FUZZER) $(CORPUS) \
		-artifact_prefix=$(ARTIFACTS)/ \
		-max_len=$(MAX_LEN) \
		-jobs=$(JOBS) \
		-workers=$(JOBS) \
		$(if $(filter-out 0,$(MAX_TIME)),-max_total_time=$(MAX_TIME))

# Generate seed corpus from existing test models
seed: | $(CORPUS)
	@echo "Seeding corpus from test models..."
	@for f in ../../../models/Cube/Cube.gltf \
	          ../../../models/Cube/Cube.glb; do \
		if [ -f "$$f" ]; then \
			cp "$$f" $(CORPUS)/; \
			echo "  Added: $$f"; \
		fi; \
	done
	@# Add a minimal valid glTF JSON
	@echo '{"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"name":"n"}]}' > $(CORPUS)/minimal.gltf
	@# Add a minimal valid GLB (header + empty JSON chunk)
	@printf 'glTF\x02\x00\x00\x00\x1c\x00\x00\x00\x04\x00\x00\x00JSON{}  ' > $(CORPUS)/minimal.glb
	@# Add edge cases
	@echo '{}' > $(CORPUS)/empty_object.gltf
	@echo '{"asset":{"version":"2.0"}}' > $(CORPUS)/asset_only.gltf
	@echo "Corpus: $$(ls $(CORPUS) | wc -l) files"

$(CORPUS):
	mkdir -p $(CORPUS)

$(ARTIFACTS):
	mkdir -p $(ARTIFACTS)

clean:
	rm -f $(FUZZER)
	rm -rf $(CORPUS) $(ARTIFACTS)
