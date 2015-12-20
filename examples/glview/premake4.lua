solution "glview"
	-- location ( "build" )
	configurations { "Debug", "Release" }
	platforms {"native", "x64", "x32"}
	
	project "glview"

		kind "ConsoleApp"
		language "C++"
		files { "glview.cc", "trackball.cc" }
		includedirs { "./" }
		includedirs { "../../" }

		configuration { "linux" }
			 linkoptions { "`pkg-config --libs glfw3`" }
			 links { "GL", "GLU", "m", "GLEW" }

		configuration { "windows" }
			 links { "glfw3", "gdi32", "winmm", "user32", "GLEW", "glu32","opengl32", "kernel32" }
			 defines { "_CRT_SECURE_NO_WARNINGS" }

		configuration { "macosx" }
         includedirs { "/usr/local/include" }
         buildoptions { "-Wno-deprecated-declarations" }
         libdirs { "/usr/local/lib" }
			links { "glfw3", "GLEW" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}

