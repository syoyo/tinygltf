solution "basic_viewer"
   -- location ( "build" )
   configurations { "Debug", "Release" }
   platforms {"native", "x64", "x32"}
   
   project "basic_viewer"

      kind "ConsoleApp"
      language "C++"
	  cppdialect "C++11"
      files { "main.cpp", "shaders.cpp", "window.cpp" }
      includedirs { "./" }
      includedirs { "../../" }
      includedirs { "../common/glm" }

      configuration { "linux" }
         linkoptions { "`pkg-config --libs glfw3`" }
         links { "GL", "GLU", "m", "GLEW", "X11", "Xrandr", "Xinerama", "Xi", "Xxf86vm", "Xcursor", "dl" }

      configuration { "windows" }
         -- Edit path to glew and GLFW3 fit to your environment.
         includedirs { "../../../../local/glew-1.13.0/include/" }
         includedirs { "../../../../local/glfw-3.2.bin.WIN32/include/" }
         libdirs { "../../../../local/glew-1.13.0/lib/Release/Win32/" }
         libdirs { "../../../../local/glfw-3.2.bin.WIN32/lib-vc2013/" }
         links { "glfw3", "gdi32", "winmm", "user32", "glew32", "glu32","opengl32", "kernel32" }
         defines { "_CRT_SECURE_NO_WARNINGS" }

      configuration { "macosx" }
         includedirs { "/usr/local/include" }
         buildoptions { "-Wno-deprecated-declarations" }
         libdirs { "/usr/local/lib" }
         links { "glfw", "GLEW" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }

      configuration "Debug"
         defines { "DEBUG" }
		 symbols "On"
		 warnings "Extra"

      configuration "Release"
         defines { "NDEBUG" }
		 optimize "On"
		 warnings "Extra"
