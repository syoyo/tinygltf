#-*-cmake-*-
#
# yue.nicholas@gmail.com
#
# This auxiliary CMake file helps in find the glfw3 headers and libraries
#
# GLFW3_FOUND            set if glfw3 is found.
# GLFW3_INCLUDE_DIR      glfw3's include directory
# GLFW3_LIBRARY_DIR      glfw3's library directory
# GLFW3_LIBRARIES        all glfw3 libraries

FIND_PACKAGE (Threads)

FIND_PACKAGE ( PackageHandleStandardArgs )

FIND_PATH( GLFW3_LOCATION include/GLFW/glfw3.h
  "$ENV{GLFW3_HOME}"
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS ( GLFW3
  REQUIRED_VARS GLFW3_LOCATION
  )

IF (GLFW3_FOUND)
  SET( GLFW3_INCLUDE_DIR "${GLFW3_LOCATION}/include" CACHE STRING "GLFW3 include path")
  IF (GLFW3_USE_STATIC_LIBS)
    FIND_LIBRARY ( GLFW3_glfw_LIBRARY  libglfw3.a  ${GLFW3_LOCATION}/lib
	  )
  ELSE (GLFW3_USE_STATIC_LIBS)
	# On windows build, we need to look for glfw3
	IF (WIN32)
	  SET ( GLFW3_LIBRARY_NAME glfw3 )
	ELSE ()
	  SET ( GLFW3_LIBRARY_NAME glfw )
	ENDIF()
    FIND_LIBRARY ( GLFW3_glfw_LIBRARY ${GLFW3_LIBRARY_NAME} ${GLFW3_LOCATION}/lib
	  )
  ENDIF (GLFW3_USE_STATIC_LIBS)

  IF (APPLE)
	FIND_LIBRARY ( COCOA_LIBRARY Cocoa )
	FIND_LIBRARY ( IOKIT_LIBRARY IOKit )
	FIND_LIBRARY ( COREVIDEO_LIBRARY CoreVideo )
  ELSEIF (UNIX AND NOT APPLE)
	SET ( GLFW3_REQUIRED_X11_LIBRARIES
	  X11
      Xi
      Xrandr
      Xinerama
      Xcursor
      Xxf86vm
	  m
	  ${CMAKE_DL_LIBS}
	  ${CMAKE_THREAD_LIBS_INIT}
      )
  ENDIF ()
  
  SET ( GLFW3_LIBRARIES
	${OPENGL_gl_LIBRARY}
	${OPENGL_glu_LIBRARY}
	${GLFW3_glfw_LIBRARY}
	# UNIX
	${GLFW3_REQUIRED_X11_LIBRARIES}
	# APPLE
	${COCOA_LIBRARY}
	${IOKIT_LIBRARY}
	${COREVIDEO_LIBRARY}
	CACHE STRING "GLFW3 required libraries"
	)
  
ENDIF ()
