sources = {
   "test.cc",
   }

-- premake4.lua
solution "TinyGLTFLoaderSolution"
   configurations { "Release", "Debug" }

   if (os.is("windows")) then
      platforms { "x32", "x64" }
   else
      platforms { "native", "x32", "x64" }
   end

   -- A project defines one build target
   project "tinygltfloader"
      kind "ConsoleApp"
      language "C++"
      files { sources }

      configuration "Debug"
         defines { "DEBUG" } -- -DDEBUG
         flags { "Symbols" }
         targetname "test_tinygltfloader_debug"

      configuration "Release"
         -- defines { "NDEBUG" } -- -NDEBUG
         flags { "Symbols", "Optimize" }
         targetname "test_tinygltfloader"
