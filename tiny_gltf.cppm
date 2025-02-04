module;
#ifdef _WIN32

// issue 143.
// Define NOMINMAX to avoid min/max defines,
// but undef it after included Windows.h
#ifndef NOMINMAX
#define TINYGLTF_INTERNAL_NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define TINYGLTF_INTERNAL_WIN32_LEAN_AND_MEAN
#endif
#ifndef __MINGW32__
#include <Windows.h>  // include API for expanding a file path
#else
#include <windows.h>
#endif

#ifdef TINYGLTF_INTERNAL_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#if defined(TINYGLTF_INTERNAL_NOMINMAX)
#undef NOMINMAX
#endif

#if defined(__GLIBCXX__)  // mingw

#include <fcntl.h>  // _O_RDONLY

#include <ext/stdio_filebuf.h>  // fstream (all sorts of IO stuff) + stdio_filebuf (=streambuf)

#endif

#elif !defined(__ANDROID__) && !defined(__OpenBSD__)
// #include <wordexp.h>
#endif
#include <assert.h>

#ifndef TINYGLTF_NO_STB_IMAGE
#ifndef TINYGLTF_NO_INCLUDE_STB_IMAGE
#include "stb_image.h"
#endif
#endif

#ifndef TINYGLTF_NO_STB_IMAGE_WRITE
#ifndef TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "stb_image_write.h"
#endif
#endif

export module tiny_gltf;

import std;
import std.compat;
// Force to use nlohmann json
import nlohmann.json;

#define TINYGLTF_USE_MODULE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

export extern "C++" {
    #include "tiny_gltf.h"
}
