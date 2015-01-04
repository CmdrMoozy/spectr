CMAKE_MINIMUM_REQUIRED(VERSION 2.8.9)
PROJECT(spectr)

SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_VERBOSE_MAKEFILE OFF)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

# Define what dependencies we have.

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")

FIND_PACKAGE(Mad REQUIRED)
FIND_PACKAGE(PkgConfig REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)

PKG_SEARCH_MODULE(GLFW REQUIRED glfw3)

FIND_PACKAGE(Freetype REQUIRED)

# Set INCLUDE and CFLAGS

IF(NOT CMAKE_BUILD_TYPE)
	MESSAGE(STATUS "No CMAKE_BUILD_TYPE specified; defaulting to Release.")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF()

SET(DBG_FLAGS "-D_XOPEN_SOURCE=700 -rdynamic -Wall -Wextra -ansi -pedantic")
SET(DBG_FLAGS "${DBG_FLAGS} -Wshadow -Wpointer-arith -Wcast-qual -O0 -std=c11")
SET(DBG_FLAGS "${DBG_FLAGS} -DSPECTR_DEBUG")

SET(REL_FLAGS "-D_XOPEN_SOURCE=700 -Wall -Wextra -ansi -pedantic -Wshadow")
SET(REL_FLAGS "${REL_FLAGS} -Wpointer-arith -Wcast-qual -fomit-frame-pointer")
SET(REL_FLAGS "${REL_FLAGS} -std=c11")

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DBG_FLAGS}")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${REL_FLAGS}")

ADD_DEFINITIONS(-DGL_GLEXT_PROTOTYPES)

INCLUDE_DIRECTORIES("src")
INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS})

# Define our project's source files.

SET(spectr_SOURCES

	src/spectr/spectr.c

	src/spectr/config.h
	src/spectr/constants.h
	src/spectr/types.h

	src/spectr/decoding/decode.c
	src/spectr/decoding/decode.h
	src/spectr/decoding/ftype.c
	src/spectr/decoding/ftype.h
	src/spectr/decoding/raw.c
	src/spectr/decoding/raw.h
	src/spectr/decoding/stat.c
	src/spectr/decoding/stat.h

	src/spectr/decoding/quirks/mp3.c
	src/spectr/decoding/quirks/mp3.h

	src/spectr/rendering/glinit.c
	src/spectr/rendering/glinit.h
	src/spectr/rendering/render.c
	src/spectr/rendering/render.h
	src/spectr/rendering/text.c
	src/spectr/rendering/text.h

	src/spectr/transform/attr.c
	src/spectr/transform/attr.h
	src/spectr/transform/fourier.c
	src/spectr/transform/fourier.h

	src/spectr/util/bitwise.c
	src/spectr/util/bitwise.h
	src/spectr/util/complex.c
	src/spectr/util/complex.h
	src/spectr/util/fonts.c
	src/spectr/util/fonts.h
	src/spectr/util/math.c
	src/spectr/util/math.h
	src/spectr/util/path.c
	src/spectr/util/path.h

)

# Build our project!

ADD_EXECUTABLE(spectr ${spectr_SOURCES})
TARGET_LINK_LIBRARIES(spectr m ${MAD_LIBRARIES}
	${OPENGL_LIBRARIES} ${GLFW_LIBRARIES} ${FREETYPE_LIBRARIES})

ADD_CUSTOM_COMMAND(TARGET spectr PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:spectr>/fonts)
