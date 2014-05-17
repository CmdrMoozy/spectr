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



# Set INCLUDE and CFLAGS

IF(NOT CMAKE_BUILD_TYPE)
	MESSAGE(STATUS "No CMAKE_BUILD_TYPE specified; defaulting to Release.")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF()

SET(DBG_FLAGS "-rdynamic -Wall -Wextra -ansi -pedantic -Wshadow")
SET(DBG_FLAGS "${DBG_FLAGS} -Wpointer-arith -Wcast-qual -O0")

SET(REL_FLAGS "-Wall -Wextra -ansi -pedantic -Wshadow -Wpointer-arith")
SET(REL_FLAGS "${REL_FLAGS} -Wcast-qual -fomit-frame-pointer")

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DBG_FLAGS}")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${REL_FLAGS}")

INCLUDE_DIRECTORIES("src")

# Define our project's source files.

SET(spectr_HEADERS



)

SET(spectr_SOURCES

	src/spectr.c

)

# Build our project!

ADD_EXECUTABLE(spectr ${spectr_SOURCES})
