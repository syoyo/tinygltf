# benchmark/Makefile — Build and run tinygltf v3 benchmarks
#
# Targets:
#   make           — build gen_synthetic + bench_v3
#   make generate  — generate synthetic test scenes
#   make run       — run benchmarks on all generated scenes
#   make report    — run benchmarks and produce CSV report
#   make clean     — remove binaries and generated scenes

CXX       ?= g++
CXXFLAGS  ?= -O2 -std=c++17 -Wall -Wextra -Wno-unused-function
CXXFLAGS  += -fno-rtti -fno-exceptions
INCLUDES  = -I..

BINDIR    = .
GEN       = $(BINDIR)/gen_synthetic
BENCH_V3  = $(BINDIR)/bench_v3

# Iteration counts
ITERATIONS ?= 10
WARMUP     ?= 2
PREFIX     ?= synthetic

.PHONY: all generate run report clean

all: $(GEN) $(BENCH_V3)

$(GEN): gen_synthetic.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BENCH_V3): bench_v3.cpp ../tiny_gltf_v3.h ../tinygltf_json.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $<

# Generate synthetic scenes of varying sizes
generate: $(GEN)
	@echo "=== Generating synthetic scenes ==="
	./$(GEN) --prefix $(PREFIX)
	@echo ""
	@echo "Generated files (binary + GLB):"
	@ls -lh $(PREFIX)_*.gltf $(PREFIX)_*.glb $(PREFIX)_*.bin 2>/dev/null || true

# Run benchmarks on all generated scenes
run: $(BENCH_V3) generate
	@echo ""
	@echo "================================================================="
	@echo "  tinygltf v3 Benchmark"
	@echo "================================================================="
	@echo ""
	@for f in $(PREFIX)_*.glb $(PREFIX)_*.gltf; do \
		if [ -f "$$f" ]; then \
			./$(BENCH_V3) "$$f" --iterations $(ITERATIONS) --warmup $(WARMUP); \
			echo ""; \
		fi; \
	done

# Run benchmarks and produce CSV report
report: $(BENCH_V3) generate
	@echo "file,size_bytes,iterations,parse_min_ms,parse_max_ms,parse_avg_ms,parse_median_ms,throughput_mbs,arena_peak_bytes,meshes,nodes,accessors,materials,animations" > benchmark_report.csv
	@for f in $(PREFIX)_*.glb $(PREFIX)_*.gltf; do \
		if [ -f "$$f" ]; then \
			./$(BENCH_V3) "$$f" --iterations $(ITERATIONS) --warmup $(WARMUP) --csv | tail -1 >> benchmark_report.csv; \
		fi; \
	done
	@echo "=== Report written to benchmark_report.csv ==="
	@cat benchmark_report.csv | column -t -s,

clean:
	rm -f $(GEN) $(BENCH_V3)
	rm -f $(PREFIX)_*.gltf $(PREFIX)_*.glb $(PREFIX)_*.bin
	rm -f benchmark_report.csv
