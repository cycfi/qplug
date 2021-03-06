###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(target ${PLUG_NAME}_au_plugin)
include(${QPLUG_ROOT}/cmake/derived.cmake)

if (CMAKE_BUILD_TYPE MATCHES Debug)
   add_compile_definitions(NDEBUG=1)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
endif()

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.61 REQUIRED)

if (NOT DEFINED PLUG_ICON_FONT)
   set(PLUG_ICON_FONT ${QPLUG_ROOT}/resources/fonts/elements_basic.ttf)
endif()

set(QPLUG_FONTS
   ${PLUG_ICON_FONT}
   ${QPLUG_ROOT}/resources/fonts/OpenSans-Bold.ttf
   ${QPLUG_ROOT}/resources/fonts/OpenSans-Light.ttf
   ${QPLUG_ROOT}/resources/fonts/OpenSans-Regular.ttf
   ${QPLUG_ROOT}/resources/fonts/Roboto-Bold.ttf
   ${QPLUG_ROOT}/resources/fonts/Roboto-Light.ttf
   ${QPLUG_ROOT}/resources/fonts/Roboto-Regular.ttf
)

set(QPLUG_RESOURCES
   ${QPLUG_FONTS}
   ${PLUG_RESOURCES}
)

find_library(AUDIOUNIT_LIBRARY AudioUnit REQUIRED)
find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
find_library(COREMIDI_LIBRARY CoreMIDI REQUIRED)
find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
find_library(ACCELERATE_LIBRARY Accelerate REQUIRED)
find_library(COREGRAPHICS_LIBRARY CoreGraphics REQUIRED)
find_library(APPKIT_LIBRARY AppKit REQUIRED)
find_library(AUDIOTOOLBOX AudioToolbox)

set(QPLUG_DEPENDENCIES
   ${QPLUG_DEPENDENCIES}
   ${COREAUDIO_LIBRARY}
   ${COREMIDI_LIBRARY}
   ${COREFOUNDATION_LIBRARY}
   ${FOUNDATION_LIBRARY}
   ${AUDIOUNIT_LIBRARY}
   ${ACCELERATE_LIBRARY}
   ${COREGRAPHICS_LIBRARY}
   ${APPKIT_LIBRARY}
   ${AUDIOTOOLBOX}
   elements
   libq
)

configure_file(
   ${QPLUG_ROOT}/cmake/factory.cpp.in
   factory.cpp
)

set (QPLUG_SOURCES
   ${QPLUG_SOURCES}
   ${CMAKE_CURRENT_BINARY_DIR}/factory.cpp
)

add_library(${target} MODULE
   ${PLUG_SOURCES}
   ${QPLUG_SOURCES}
   ${AU_SOURCES}
   ${IPLUG2_LIB_SOURCES}
   ${QPLUG_RESOURCES}
)

target_compile_definitions(${target}
   PUBLIC
   IPLUG2=1
   AU_API=1
   NO_IGRAPHICS=1
   IPLUG_DSP=1
   SAMPLE_TYPE_FLOAT=1
   MSGPACK_DISABLE_LEGACY_NIL=1
)

set_source_files_properties(
   ${QPLUG_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources
)

target_include_directories(${target}
   PUBLIC
   ${PLUG_INCLUDE_DIRECTORIES}
   ${IPLUG2_INCLUDE_DIRS}
   ${QPLUG_INCLUDE_DIRS}
   ${AU_INCLUDE_DIRS}
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}
   ${QPLUG_ROOT}/lib/infra/include
   ${QPLUG_ROOT}/external/msgpack-c/include
   ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${target}
   PRIVATE
   ${QPLUG_DEPENDENCIES}
)

configure_file(
   ${QPLUG_ROOT}/cmake/config.h.in
   config.h
)

set_target_properties(${target}
   PROPERTIES
   BUNDLE true
   BUNDLE_EXTENSION "component"
   OUTPUT_NAME "${PLUG_NAME}"
   MACOSX_BUNDLE_INFO_PLIST "${QPLUG_ROOT}/cmake/au.plist.in"
)


