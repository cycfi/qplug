###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################################################
# elements

set(ELEMENTS_NO_EXAMPLES YES)

if (NOT DEFINED ELEMENTS_ROOT)
  set(ELEMENTS_ROOT ${CMAKE_CURRENT_LIST_DIR}/../elements)
endif()

add_subdirectory(${ELEMENTS_ROOT} "${CMAKE_CURRENT_BINARY_DIR}/elements")

###############################################################################
# Q

if (NOT DEFINED Q_ROOT)
  set(Q_ROOT ${CMAKE_CURRENT_LIST_DIR}/../q)
endif()

set(Q_NO_EXAMPLES YES)

add_subdirectory(${Q_ROOT} "${CMAKE_CURRENT_BINARY_DIR}/q")

###############################################################################
# qplug

set(QPLUG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
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


