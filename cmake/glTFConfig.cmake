# -*- cmake -*-
# - Find glTF

# glTF_INCLUDE_DIR      glTF's include directory


FIND_PACKAGE ( PackageHandleStandardArgs )

SET ( glTF_INCLUDE_DIR "${glTF_DIR}/../include" CACHE STRING "glTF include directory")

FIND_FILE ( glTF_HEADER tiny_gltf.h PATHS ${glTF_INCLUDE_DIR} )

IF (glTF_HEADER)
  # MESSAGE ( STATUS "FOUND tiny_gltf.h, glTF_INCLUDE_DIR = ${glTF_INCLUDE_DIR}")
ELSE ()
  MESSAGE ( FATAL_ERROR "Unable to find tiny_gltf.h, glTF_INCLUDE_DIR = ${glTF_INCLUDE_DIR}")
ENDIF ()
