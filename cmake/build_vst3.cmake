###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(target ${PLUG_NAME}_vst3_plugin)
include(${QPLUG_ROOT}/cmake/derived.cmake)

if (CMAKE_BUILD_TYPE MATCHES Debug)
   add_compile_definitions(NDEBUG=1)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")

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

if (APPLE)
   find_library(AUDIOUNIT_LIBRARY AudioUnit REQUIRED)
   find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
   find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
   find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
   find_library(ACCELERATE_LIBRARY Accelerate REQUIRED)
   find_library(COREGRAPHICS_LIBRARY CoreGraphics REQUIRED)
   find_library(APPKIT_LIBRARY AppKit REQUIRED)
   find_library(AUDIOTOOLBOX AudioToolbox)

   set(QPLUG_DEPENDENCIES
      ${QPLUG_DEPENDENCIES}
      ${COREAUDIO_LIBRARY}
      ${COREFOUNDATION_LIBRARY}
      ${FOUNDATION_LIBRARY}
      ${AUDIOUNIT_LIBRARY}
      ${ACCELERATE_LIBRARY}
      ${COREGRAPHICS_LIBRARY}
      ${APPKIT_LIBRARY}
      ${AUDIOTOOLBOX}
      libelements
   )
endif()

add_compile_definitions(VST3_API=1)
add_compile_definitions(NO_IGRAPHICS=1)
add_compile_definitions(IPLUG_DSP=1)

add_library(${target} MODULE
   ${PLUG_SOURCES}
   ${VST3_SOURCES}
   ${IPLUG2_LIB_SOURCES}
   ${QPLUG_RESOURCES}
)

set_source_files_properties(
   ${QPLUG_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources
)

target_include_directories(${target}
   PUBLIC
   ${PLUG_INCLUDE_DIRECTORIES}
   ${VST3_ROOT}
   ${IPLUG2_INCLUDE_DIRS}
   ${VST3_INCLUDE_DIRS}
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${target}
   PRIVATE
   ${QPLUG_DEPENDENCIES}
)

configure_file(
   ${QPLUG_ROOT}/cmake/config.h.in
   config.h
)

if (APPLE)
   set_target_properties(${target}
      PROPERTIES
      BUNDLE true
      BUNDLE_EXTENSION "vst3"
      OUTPUT_NAME "${PLUG_NAME}"
      MACOSX_BUNDLE_INFO_PLIST "${QPLUG_ROOT}/cmake/vst3.plist.in"
   )

   add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND "${QPLUG_TOOLS}/macos/validator" "$<TARGET_BUNDLE_DIR:${target}>"
   )

   add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_BUNDLE_DIR:${target}> ~/Library/Audio/Plug-ins/VST3/${PLUG_NAME}.vst3
   )
endif()





