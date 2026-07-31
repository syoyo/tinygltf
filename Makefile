# tinygltf v3 (C) — convenience build entry point.
#
#   make            — build all v3 C unit testers
#   make test       — build and run all v3 C unit testers
#   make clean      — remove test binaries

all:
	$(MAKE) -C tests all

test: all
	$(MAKE) -C tests run

clean:
	$(MAKE) -C tests clean
