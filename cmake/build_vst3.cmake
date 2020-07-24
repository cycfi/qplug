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
      elements
      libq
   )
endif()

if (WIN32)
   set(QPLUG_DEPENDENCIES
      ${QPLUG_DEPENDENCIES}
      Delayimp.lib
      elements
      libq
   )
endif()

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
   ${VST3_SOURCES}
   ${IPLUG2_LIB_SOURCES}
   ${QPLUG_RESOURCES}
)

# Get rid of certain warnings
target_compile_options(${target} PRIVATE
   $<$<CXX_COMPILER_ID:GNU>: -Wextra -Wpedantic -ftemplate-backtrace-limit=0>
   $<$<CXX_COMPILER_ID:Clang>: -Wpedantic -ftemplate-backtrace-limit=0>
   $<$<CXX_COMPILER_ID:MSVC>: /wd4068 /wd4244 /wd4305 /wd4996 /wd4267 /wd4018>
)

target_compile_definitions(${target}
   PUBLIC
   IPLUG2=1
   VST3_API=1
   NO_IGRAPHICS=1
   IPLUG_DSP=1
   SAMPLE_TYPE_FLOAT=1
   MSGPACK_DISABLE_LEGACY_NIL=1
)

if (APPLE)
   set_source_files_properties(
      ${QPLUG_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources
   )
endif()

target_include_directories(${target}
   PUBLIC
   ${PLUG_INCLUDE_DIRECTORIES}
   ${VST3_ROOT}
   ${IPLUG2_INCLUDE_DIRS}
   ${QPLUG_INCLUDE_DIRS}
   ${VST3_INCLUDE_DIRS}
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

if (APPLE)
   set_target_properties(${target}
      PROPERTIES
      BUNDLE true
      BUNDLE_EXTENSION "vst3"
      OUTPUT_NAME "${PLUG_NAME}"
      MACOSX_BUNDLE_INFO_PLIST "${QPLUG_ROOT}/cmake/vst3.plist.in"
   )

   add_custom_target(
      ${target}_test
      COMMAND "${QPLUG_TOOLS}/macos/validator" "$<TARGET_BUNDLE_DIR:${target}>"
      DEPENDS ${target}
      VERBATIM
   )

   add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_BUNDLE_DIR:${target}> ~/Library/Audio/Plug-ins/VST3/${PLUG_NAME}.vst3
   )
endif()

if (WIN32)

   if (CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(BUILD_PREFIX x86_64-win)
      set(VST_VALIDATOR "${QPLUG_TOOLS}/windows/x64/validator.exe")
   elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(BUILD_PREFIX x86-win)
      set(VST_VALIDATOR "${QPLUG_TOOLS}/windows/x86/validator.exe")
   endif()

   set(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${PLUG_NAME}.vst3/Contents/${BUILD_PREFIX}")

   set_target_properties(${target}
      PROPERTIES
      SUFFIX ".vst3"
      OUTPUT_NAME "${PLUG_NAME}"
      LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}
      LINK_FLAGS "/EXPORT:GetPluginFactory /DELAYLOAD:cairo.dll /DELAYLOAD:freetype.dll /DELAYLOAD:fontconfig.dll"
   )

   if (CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits?
      set(CAIRO_DLL ${ELEMENTS_ROOT}/lib/external/cairo/lib/x64/cairo.dll)
      set(FREETYPE_DLL ${ELEMENTS_ROOT}/lib/external/freetype/win64/freetype.dll)
      set(FONTCONFIG_DLL ${ELEMENTS_ROOT}/lib/external/fontconfig/x64/fontconfig.dll)
      set(ICONV_DLL ${ELEMENTS_ROOT}/lib/external/fontconfig/x64/libiconv.dll)
      set(XML2 ${ELEMENTS_ROOT}/lib/external/fontconfig/x64/libxml2.dll)
   else()
      set(CAIRO_DLL ${ELEMENTS_ROOT}/lib/external/cairo/lib/x86/cairo.dll)
      set(FREETYPE_DLL ${ELEMENTS_ROOT}/lib/external/freetype/win32/freetype.dll)
      set(FONTCONFIG_DLL ${ELEMENTS_ROOT}/lib/external/fontconfig/x86/fontconfig.dll)
      set(ICONV_DLL ${ELEMENTS_ROOT}/lib/external/fontconfig/x86/libiconv.dll)
      set(XML2 ${ELEMENTS_ROOT}/lib/external/fontconfig/x86/libxml2.dll)
   endif()

   file(COPY ${CAIRO_DLL} DESTINATION "${OUTPUT_DIRECTORY}")
   file(COPY ${FREETYPE_DLL} DESTINATION "${OUTPUT_DIRECTORY}")
   file(COPY ${FONTCONFIG_DLL} DESTINATION "${OUTPUT_DIRECTORY}")
   file(COPY ${ICONV_DLL} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
   file(COPY ${XML2} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
   add_custom_target(
      ${target}_test
      COMMAND "${VST_VALIDATOR}" "${CMAKE_BINARY_DIR}/${PLUG_NAME}.vst3"
      DEPENDS ${target}
      VERBATIM
   )

   target_compile_definitions(${target}
      PRIVATE "-DQPLUG_DLL_LINK_ORDER=\"cairo freetype libiconv libxml2 fontconfig\""
   )

   file(
      COPY ${QPLUG_RESOURCES}
      DESTINATION "${CMAKE_BINARY_DIR}/${PLUG_NAME}.vst3/Contents/Resources"
   )

endif()


