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
# qplug
set(QPLUG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(QPLUG_TOOLS "${QPLUG_ROOT}/external/tools")
include(cmake/defaults.cmake)

include(cmake/external.cmake)
add_subdirectory(examples)


