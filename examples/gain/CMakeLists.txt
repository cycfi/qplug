###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(PLUG_NAME "IPlugEffect")
set(PLUG_MFR "Cycfi Research")
set(PLUG_MFR_ID Cycf)
set(PLUG_COPYRIGHT_STR "Copyright 2019 Cycfi Research Inc")
set(BUNDLE_MFR "CycfiInc")

set(PLUG_SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/gain.cpp
)

include(${QPLUG_ROOT}/cmake/build_vst3.cmake)

