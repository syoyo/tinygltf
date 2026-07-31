newoption {
   trigger = "with-gtk3nfd",
   description = "Build with native file dialog support(GTK3 required. Linux only)"
}

newoption {
   trigger = "asan",
   description = "Enable Address Sanitizer(gcc5+ ang clang only)"
}

sources = {
   "stbi-impl.cc",
   "main.cc",
   "render.cc",
   "render-config.cc",
   "obj-loader.cc",
   "gltf-loader.cc",
   "matrix.cc",
   "../common/trackball.cc",
   "../common/imgui/imgui.cpp",
   "../common/imgui/imgui_draw.cpp",
   "../common/imgui/imgui_impl_btgui.cpp",
   "../common/imgui/ImGuizmo.cpp",
   }

solution "NanoSGSolution"
   configurations { "Release", "Debug" }

   if os.is("Windows") then
      platforms { "x64", "x32" }
   else
      platforms { "native", "x64", "x32" }
   end


   -- RootDir for OpenGLWindow
   projectRootDir = os.getcwd() .. "/../common/"
   dofile ("../common/findOpenGLGlewGlut.lua")
   initOpenGL()
   initGlew()

   -- Use c++11
   flags { "c++11" }

   -- A project defines one build target
   project "viwewer"
      kind "ConsoleApp"
      language "C++"
      files { sources }

      includedirs { "./", "../../" }
      includedirs { "../common" }
      includedirs { "../common/imgui" }
      includedirs { "../common/glm" }
      --includedirs { "../common/nativefiledialog/src/include" }

      if _OPTIONS['asan'] then
         buildoptions { "-fsanitize=address" }
         linkoptions { "-fsanitize=address" }
      end

      if os.is("Windows") then
         warnings "Extra" -- /W4

         defines { "NOMINMAX" }
         defines { "USE_NATIVEFILEDIALOG" }
         buildoptions { "/W4" } -- raise compile error level.
         files{
            "../common/OpenGLWindow/Win32OpenGLWindow.cpp",
            "../common/OpenGLWindow/Win32OpenGLWindow.h",
            "../common/OpenGLWindow/Win32Window.cpp",
            "../common/OpenGLWindow/Win32Window.h",
            }
         includedirs { "./../common/nativefiledialog/src/include" }
         files { "../common/nativefiledialog/src/nfd_common.c",
                 "../common/nativefiledialog/src/nfd_win.cpp" }
      end
      if os.is("Linux") then
         files {
            "../common/OpenGLWindow/X11OpenGLWindow.cpp",
            "../common/OpenGLWindow/X11OpenGLWindows.h"
            }
         links {"X11", "pthread", "dl"}
         if _OPTIONS["with-gtk3nfd"] then
            defines { "USE_NATIVEFILEDIALOG" }
            includedirs { "./../common/nativefiledialog/src/include" }
            files { "../common/nativefiledialog/src/nfd_gtk.c",
                    "../common/nativefiledialog/src/nfd_common.c"
                  }
            buildoptions { "`pkg-config --cflags gtk+-3.0`" }
            linkoptions { "`pkg-config --libs gtk+-3.0`" }
         end
      end
      if os.is("MacOSX") then
         defines { "USE_NATIVEFILEDIALOG" }
         links {"Cocoa.framework"}
         files {
                "../common/OpenGLWindow/MacOpenGLWindow.h",
                "../common/OpenGLWindow/MacOpenGLWindow.mm",
               }
         includedirs { "./../common/nativefiledialog/src/include" }
         files { "../common/nativefiledialog/src/nfd_cocoa.m",
                 "../common/nativefiledialog/src/nfd_common.c" }
      end

      configuration "Debug"
         defines { "DEBUG" } -- -DDEBUG
         symbols "On"
         targetname "view_debug"

      configuration "Release"
         -- defines { "NDEBUG" } -- -NDEBUG
         symbols "On"
	 optimize "On"
         targetname "view"
