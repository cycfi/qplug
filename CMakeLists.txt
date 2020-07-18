###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(qplug)
set(QPLUG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

option(QPLUG_BUILD_TEST "Build QPlug library tests" ON)

###############################################################################
# elements

set(ELEMENTS_BUILD_EXAMPLES OFF)
add_subdirectory(${QPLUG_ROOT}/lib/elements)

###############################################################################
# Q

set(Q_BUILD_EXAMPLES OFF CACHE BOOL "")
set(Q_BUILD_TEST OFF CACHE BOOL "")
set(Q_BUILD_IO OFF CACHE BOOL "")
add_subdirectory(${QPLUG_ROOT}/lib/q)

###############################################################################
# Boost

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.61 REQUIRED)

###############################################################################
# qplug

set(QPLUG_TOOLS "${QPLUG_ROOT}/external/tools")
include(${QPLUG_ROOT}/cmake/defaults.cmake)

set (QPLUG_SOURCES
   ${QPLUG_ROOT}/lib/src/processor.cpp
   ${QPLUG_ROOT}/lib/src/controller.cpp
   ${QPLUG_ROOT}/lib/src/iplug2/iplug2_plugin.cpp
)

set (QPLUG_INCLUDE_DIRS
   ${QPLUG_ROOT}/lib/include
)

include(${QPLUG_ROOT}/cmake/external.cmake)

###############################################################################
# qplug tests

if (QPLUG_BUILD_TEST)
   add_subdirectory(test)
endif()
