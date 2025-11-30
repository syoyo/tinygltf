# Repository Guidelines

## Project Structure & Module Organization
- Core library lives in `tiny_gltf.h` (header-only) with `tiny_gltf.cc` provided for the amalgamated implementation flags. Keep public API updates localized and documented.
- Example viewers and utilities sit under `examples/`; use them as references for loading, validation, and WASM builds. Temporary build outputs belong in `build/` (git-ignored).
- Tests reside in `tests/` with sample assets in `data/` and `models/`; avoid committing generated binaries in `build/`, `tmp/`, or `tests/tester*`.

## Build, Test, and Development Commands
- Quick build of the loader example: `make` (uses clang++, C++11, optional `EXTRA_CXXFLAGS` for sanitizers).
- Unit tests: `cd tests && make && ./tester && ./tester_noexcept`.
- Parsing regression run: build `loader_example`, then `python test_runner.py` (requires local glTF-Sample-Models checkout and path update inside the script).
- CMake alternative: `cmake -S . -B build && cmake --build build` for IDE integration or non-clang toolchains.
- Lint header: `python deps/cpplint.py tiny_gltf.h`.

## Coding Style & Naming Conventions
- C++11, two-space indent, braces on the same line; mirror existing spacing and comment style in `tiny_gltf.h`.
- Prefer `std::` facilities and minimal dependencies; keep new symbols in the `tinygltf` namespace.
- Public API names stay PascalCase for types and camelCase for functions; keep enums/macros consistent with existing `TINYGLTF_*` patterns.
- Guard optional features with the established `TINYGLTF_*` defines; avoid introducing new globals without discussion.

## Testing Guidelines
- Framework: Catch2 single-header (`tests/catch.hpp`); add `TEST_CASE` blocks alongside related helpers in `tests/tester.cc`.
- Provide coverage for both exception-enabled and `TINYGLTF_NOEXCEPTION` builds; run both `tester` binaries before submitting.
- For new formats or parsing code, add assets under `tests/` or reference `data/` and note provenance.

## Commit & Pull Request Guidelines
- Commit messages: concise, present-tense imperatives mirroring existing history (e.g., “Add bounds check to images loaded from bufferviews”).
- PRs should describe the change, motivation, and testing (`tester`, `tester_noexcept`, fuzzing if relevant); link related issues.
- Include platform notes if behavior differs (Windows vs. POSIX, filesystem callbacks, WASM). Add before/after metrics when touching performance-sensitive paths.

## Security & Configuration Tips
- Handle external data defensively: validate buffer sizes, offsets, and URI handling; prefer bounded allocations.
- Keep optional callbacks (`fs::`, URI, image) robust against untrusted input; document new failure modes.
- Avoid committing sample assets with unclear licensing; reuse existing test fixtures where possible.
