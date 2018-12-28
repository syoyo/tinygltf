# -*- cmake -*-
# - Find TinyGLTF

# TinyGLTF_INCLUDE_DIR      TinyGLTF's include directory


FIND_PACKAGE ( PackageHandleStandardArgs )

SET ( TinyGLTF_INCLUDE_DIR "${TinyGLTF_DIR}/../include" CACHE STRING "TinyGLTF include directory")

FIND_FILE ( TinyGLTF_HEADER tiny_gltf.h PATHS ${TinyGLTF_INCLUDE_DIR} )

IF (NOT TinyGLTF_HEADER)
  MESSAGE ( FATAL_ERROR "Unable to find tiny_gltf.h, TinyGLTF_INCLUDE_DIR = ${TinyGLTF_INCLUDE_DIR}")
ENDIF ()
