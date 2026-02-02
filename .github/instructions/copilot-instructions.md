# Copilot Review Instructions for TinyGLTF

This document provides guidelines for reviewing code changes in the TinyGLTF repository.

## Memory Safety

- **Buffer Overflows**: Check for proper bounds checking when accessing arrays, vectors, and buffers. Verify that buffer sizes are validated before read/write operations.
- **Null Pointer Dereferences**: Ensure all pointers are checked for null before dereferencing, especially when handling optional glTF fields.
- **Memory Leaks**: Verify proper resource management, including RAII patterns for file handles, image data, and dynamically allocated memory.
- **Use-After-Free**: Check for proper lifetime management of objects, especially when dealing with callbacks and asynchronous operations.

## Error Handling

- **File I/O**: Verify that all file operations have proper error checking and meaningful error messages.
- **JSON Parsing**: Ensure JSON parsing errors are caught and reported with helpful context about the location and nature of the error.
- **Resource Loading**: Check that failures in loading images, buffers, and other resources are properly handled and don't cause crashes.
- **Error Propagation**: Verify that errors are properly propagated through the call stack with appropriate error messages.

## glTF 2.0 Specification Compliance

- **Required Fields**: Ensure all required glTF fields are validated and present.
- **Data Types**: Verify that data types match the glTF specification (e.g., component types, accessor types).
- **Constraints**: Check that glTF constraints are enforced (e.g., valid ranges for enums, buffer stride requirements).
- **Extensions**: Verify proper handling of glTF extensions and that unknown extensions are handled gracefully.
- **Validation**: Ensure new features align with the glTF 2.0 specification from the Khronos Group.

## Cross-Platform Compatibility

- **Windows**: Check for proper handling of Windows-specific issues (path separators, line endings, file operations).
- **Linux**: Verify compatibility with various Linux distributions and compilers (GCC, Clang).
- **macOS**: Ensure macOS-specific considerations are addressed (case-sensitive filesystems, Clang compatibility).
- **Mobile Platforms**: Consider Android and iOS compatibility where applicable.
- **Endianness**: Verify proper handling of byte order when reading binary data.
- **Compiler Compatibility**: Ensure code compiles with C++11 standard and supported compilers (MSVC, GCC, Clang).

## Edge Cases in glTF Parsing

- **Empty/Minimal Files**: Verify handling of minimal valid glTF files.
- **Large Files**: Check for proper handling of large glTF files and buffers without memory exhaustion.
- **Malformed Data**: Ensure graceful handling of malformed or invalid glTF data.
- **Missing Optional Fields**: Verify correct behavior when optional glTF fields are absent.
- **Edge Values**: Check handling of boundary values (e.g., maximum buffer sizes, extreme floating-point values).
- **Base64 Encoding**: Verify proper handling of base64-encoded data URIs and invalid encodings.

## Backwards Compatibility

- **API Changes**: Ensure public API changes maintain backwards compatibility or are properly deprecated.
- **Breaking Changes**: Flag any breaking changes for major version updates and document migration paths.
- **Binary Compatibility**: Consider ABI stability for header-only library changes.
- **Default Behavior**: Verify that default behavior of existing functionality remains unchanged.

## Performance Considerations

- **Parsing Performance**: Check for unnecessary copies, redundant allocations, and inefficient algorithms in parsing logic.
- **Memory Usage**: Verify efficient memory usage, especially when loading large glTF files.
- **I/O Operations**: Ensure efficient file reading and minimize unnecessary disk access.
- **String Operations**: Check for efficient string handling (use of string_view, move semantics).
- **STL Usage**: Verify appropriate use of STL containers and algorithms.

## Documentation

- **Public API**: Ensure all public functions, classes, and methods have clear documentation comments.
- **Parameters**: Verify that function parameters are documented, including expected ranges and constraints.
- **Return Values**: Document return values and possible error conditions.
- **Examples**: Check that complex features include usage examples.
- **Changelog**: Verify that significant changes are documented in release notes or changelog.

## Testing

- **Test Coverage**: Ensure new features include appropriate unit tests or integration tests.
- **Edge Cases**: Verify that tests cover edge cases and error conditions.
- **Cross-Platform Tests**: Check that tests run on all supported platforms.
- **Regression Tests**: Ensure bug fixes include regression tests to prevent recurrence.
- **Sample Files**: Verify that changes are tested with various valid and invalid glTF sample files.

## Code Style Consistency

- **Header-Only Pattern**: Maintain the header-only library structure.
- **Naming Conventions**: Follow existing naming conventions (CamelCase for types, snake_case for functions where applicable).
- **Formatting**: Adhere to the existing code formatting style (check `.clang-format` if available).
- **Include Guards**: Verify proper include guards and header organization.
- **Namespace Usage**: Ensure proper use of the `tinygltf` namespace.
- **Comments**: Maintain consistent comment style with existing code.
- **C++11 Compliance**: Verify that code uses C++11 features appropriately and doesn't require newer standards unless specified.

## Additional Considerations

- **Third-Party Dependencies**: Minimize new dependencies; prefer existing dependencies (json.hpp, stb_image).
- **Warnings**: Ensure code compiles without warnings on supported compilers.
- **const Correctness**: Verify proper use of const for parameters and methods.
- **RAII**: Prefer RAII patterns for resource management over manual cleanup.
- **noexcept**: Use noexcept appropriately for move constructors and move assignment operators.
