###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE MATCHES Debug)
   add_compile_definitions(NDEBUG=1)
endif()

set(QPLUG_BUILD_TEST OFF CACHE BOOL "")
add_subdirectory(${QPLUG_ROOT} "${CMAKE_CURRENT_BINARY_DIR}/qplug")

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
set(ELEMENTS_ROOT ${QPLUG_ROOT}/lib/elements)

include(${QPLUG_ROOT}/cmake/external.cmake)

