###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE MATCHES Debug)
   add_compile_definitions(NDEBUG=1)
endif()

add_compile_definitions(VST3_API=1)
add_compile_definitions(NO_IGRAPHICS=1)

if (APPLE)
   find_library(AUDIOUNIT_LIBRARY AudioUnit REQUIRED)
   find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
   find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
   find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
   find_library(ACCELERATE_LIBRARY Accelerate REQUIRED)
   find_library(COREGRAPHICS_LIBRARY CoreGraphics REQUIRED)
   find_library(APPKIT_LIBRARY AppKit REQUIRED)
   find_library(AUDIOTOOLBOX AudioToolbox)
endif()

set(IPLUG2_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/external/iPlug2)
set(VST3_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/external/VST3_SDK)

set(IPLUG2_LIB_SOURCES
   ${IPLUG2_ROOT}/IPlug/IPlugAPIBase.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugParameter.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugPaths.mm
   ${IPLUG2_ROOT}/IPlug/IPlugPluginBase.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugProcessor.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugTimer.cpp
   ${IPLUG2_ROOT}/IPlug/VST3/IPlugVST3.cpp
   ${IPLUG2_ROOT}/IPlug/VST3/IPlugVST3_ProcessorBase.cpp
)

set(VST3_SOURCES
   ${VST3_ROOT}/base/source/baseiids.cpp
   ${VST3_ROOT}/base/source/fbuffer.cpp
   ${VST3_ROOT}/base/source/fdebug.cpp
   ${VST3_ROOT}/base/source/fdynlib.cpp
   ${VST3_ROOT}/base/source/fobject.cpp
   ${VST3_ROOT}/base/source/fstreamer.cpp
   ${VST3_ROOT}/base/source/fstring.cpp
   ${VST3_ROOT}/base/source/timer.cpp
   ${VST3_ROOT}/base/source/updatehandler.cpp
   ${VST3_ROOT}/base/thread/source/fcondition.cpp
   ${VST3_ROOT}/base/thread/source/flock.cpp
   ${VST3_ROOT}/pluginterfaces/base/conststringtable.cpp
   ${VST3_ROOT}/pluginterfaces/base/coreiids.cpp
   ${VST3_ROOT}/pluginterfaces/base/funknown.cpp
   ${VST3_ROOT}/pluginterfaces/base/ustring.cpp
   ${VST3_ROOT}/public.sdk/source/common/commoniids.cpp
   ${VST3_ROOT}/public.sdk/source/common/memorystream.cpp
   ${VST3_ROOT}/public.sdk/source/common/pluginview.cpp
   ${VST3_ROOT}/public.sdk/source/main/macmain.cpp
   ${VST3_ROOT}/public.sdk/source/main/pluginfactory.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstaudioeffect.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstbus.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstbypassprocessor.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstcomponent.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstcomponentbase.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstinitiids.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstnoteexpressiontypes.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstparameters.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstpresetfile.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstrepresentation.cpp
   ${VST3_ROOT}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
)

set(IPLUG2_SOURCES
   IPlugEffect.cpp
)

add_library(IPlugEffect MODULE
   ${IPLUG2_LIB_SOURCES}
   ${VST3_SOURCES}
   ${IPLUG2_SOURCES}
)

target_include_directories(IPlugEffect
   PUBLIC
   ${VST3_ROOT}
   ${IPLUG2_ROOT}/IPlug
   ${IPLUG2_ROOT}/IPlug/Extras
   ${IPLUG2_ROOT}/IPlug/VST3
   ${IPLUG2_ROOT}/WDL
   ${IPLUG2_ROOT}/IPlug/IPlugEffect
   ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(IPlugEffect
   PRIVATE
   ${COREAUDIO_LIBRARY}
   ${COREFOUNDATION_LIBRARY}
   ${FOUNDATION_LIBRARY}
   ${AUDIOUNIT_LIBRARY}
   ${ACCELERATE_LIBRARY}
   ${COREGRAPHICS_LIBRARY}
   ${APPKIT_LIBRARY}
   ${AUDIOTOOLBOX}
)

if (APPLE)
   set_target_properties(IPlugEffect
      PROPERTIES
      BUNDLE true
      BUNDLE_EXTENSION "vst3"
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/VST3-Info.plist"
   )
endif()



