sources = {
   "loader_example.cc",
   }

-- premake4.lua
solution "TinyGLTFSolution"
   configurations { "Release", "Debug" }

   if (os.is("windows")) then
      platforms { "x32", "x64" }
   else
      platforms { "native", "x32", "x64" }
   end

   -- A project defines one build target
   project "tinygltf"
      kind "ConsoleApp"
      language "C++"
      files { sources }
      flags { "C++11" }

      configuration "Debug"
         defines { "DEBUG" } -- -DDEBUG
         flags { "Symbols" }
         targetname "loader_example_tinygltf_debug"

      configuration "Release"
         -- defines { "NDEBUG" } -- -NDEBUG
         flags { "Symbols", "Optimize" }
         targetname "loader_example_tinygltf"
