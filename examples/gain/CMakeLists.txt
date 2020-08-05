###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(gain)

#------------------------------------------------------------------------------
# QPlug
#------------------------------------------------------------------------------
set (ELEMENTS_CLASS_PREFIX Gain)

if (NOT DEFINED QPLUG_ROOT)
  set(QPLUG_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
endif()

include(${QPLUG_ROOT}/cmake/qplug.cmake)

#------------------------------------------------------------------------------
# gain plugin
#------------------------------------------------------------------------------
set(PLUG_NAME "gain")
set(PLUG_MFR "Cycfi Research")
set(PLUG_MFR_ID Cycf)
set(PLUG_COPYRIGHT_STR "Copyright 2019 Cycfi Research Inc")
set(PLUG_DESCRIPTION "Gain Effects Example")
set(PLUG_UNIQUE_ID 5wGu)
set(BUNDLE_MFR "CycfiInc")
set(PLUG_WIDTH 400)
set(PLUG_HEIGHT 350)
set(AUV2_TYPE "aufx")

set(PLUG_SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/gain_controller.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/gain_processor.cpp
)

set(PLUG_HEADERS
   ${CMAKE_CURRENT_SOURCE_DIR}/gain_controller.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/gain_processor.hpp
)

set(PLUG_INCLUDE_DIRECTORIES
   ${CMAKE_CURRENT_SOURCE_DIR}
)

include(${QPLUG_ROOT}/cmake/build_vst3.cmake)

if (APPLE)
   include(${QPLUG_ROOT}/cmake/build_au.cmake)
endif()

