# swatbotics/hubo-logging/CMakeLists.txt

cmake_minimum_required(VERSION 2.6)

project(HUBO_LOGGING)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_C_FLAGS "-Wall -g")
set(CMAKE_CXX_FLAGS "-Wall -g")

set(CMAKE_C_FLAGS_DEBUG "-O")
set(CMAKE_CXX_FLAGS_DEBUG "-O")

set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_path(HUBO_ACH_INCLUDE "hubo.h" PATHS ${CMAKE_INCLUDE_PATH})
if (${HUBO_ACH_INCLUDE} STREQUAL HUBO_ACH_INCLUDE-NOTFOUND)
  message("could not find hubo-ach includes")
  set(HAVE_HUBO_ACH 0)
else (${HUBO_ACH_INCLUDE} STREQUAL HUBO_ACH_INCLUDE-NOTFOUND)
  message("found hubo-ach includes at ${HUBO_ACH_INCLUDE}")
  add_definitions(-DHAVE_HUBO_ACH)
  set(HAVE_HUBO_ACH 1)
endif (${HUBO_ACH_INCLUDE} STREQUAL HUBO_ACH_INCLUDE-NOTFOUND)

add_subdirectory(src)
