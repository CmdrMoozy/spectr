#     - To enable "release" or "debug" builds, instantiate as follows:
#
#           cmake -DCMAKE_BUILD_TYPE=Release ..		(DEFAULT)
#           cmake -DCMAKE_BUILD_TYPE=Debug ..

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

SET(spectr_HEADERS

	src/config.h
	src/constants.h
	src/types.h

	src/decoding/decode.h
	src/decoding/ftype.h
	src/decoding/raw.h
	src/decoding/stat.h

	src/decoding/quirks/mp3.h

	src/rendering/glinit.h
	src/rendering/render.h

	src/transform/attr.h
	src/transform/fourier.h

	src/util/bitwise.h
	src/util/complex.h
	src/util/fonts.h
	src/util/math.h
	src/util/path.h

)

SET(spectr_SOURCES

	src/spectr.c

	src/decoding/decode.c
	src/decoding/ftype.c
	src/decoding/raw.c
	src/decoding/stat.c

	src/decoding/quirks/mp3.c

	src/rendering/glinit.c
	src/rendering/render.c

	src/transform/attr.c
	src/transform/fourier.c

	src/util/bitwise.c
	src/util/complex.c
	src/util/fonts.c
	src/util/math.c
	src/util/path.c

)

# Build our project!

ADD_EXECUTABLE(spectr ${spectr_SOURCES})
TARGET_LINK_LIBRARIES(spectr m ${MAD_LIBRARIES}
	${OPENGL_LIBRARIES} ${GLFW_LIBRARIES} ${FREETYPE_LIBRARIES})

ADD_CUSTOM_COMMAND(TARGET spectr PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:spectr>/fonts)
