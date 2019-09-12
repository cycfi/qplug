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

set(VST3_ROOT ${QPLUG_ROOT}/external/VST3_SDK)
set(IPLUG2_ROOT ${QPLUG_ROOT}/external/iPlug2)

###############################################################################
# VST3

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
   ${IPLUG2_ROOT}/IPlug/VST3/IPlugVST3.cpp
   ${IPLUG2_ROOT}/IPlug/VST3/IPlugVST3_ProcessorBase.cpp
)

set(
   VST3_INCLUDE_DIRS ${VST3_ROOT}
   ${IPLUG2_ROOT}/IPlug/VST3
)

###############################################################################
# AU

set(AU_SOURCES
   ${IPLUG2_ROOT}/IPlug/AUv2/IPlugAU.cpp
   ${IPLUG2_ROOT}/IPlug/AUv2/IPlugAU_view_factory.mm
   ${IPLUG2_ROOT}/IPlug/AUv2/dfx-au-utilities.c
   ${IPLUG2_ROOT}/IPlug/IPlugAPIBase.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugParameter.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugPaths.mm
   ${IPLUG2_ROOT}/IPlug/IPlugPluginBase.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugProcessor.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugTimer.cpp
)

set(AU_INCLUDE_DIRS
   ${IPLUG2_ROOT}/IPlug/AUv2
)

###############################################################################
# IPlug2

set(IPLUG2_LIB_SOURCES
   ${IPLUG2_ROOT}/IPlug/IPlugAPIBase.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugParameter.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugPaths.mm
   ${IPLUG2_ROOT}/IPlug/IPlugPluginBase.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugProcessor.cpp
   ${IPLUG2_ROOT}/IPlug/IPlugTimer.cpp
)

set(IPLUG2_INCLUDE_DIRS
   ${IPLUG2_ROOT}/IPlug
   ${IPLUG2_ROOT}/IPlug/Extras
   ${IPLUG2_ROOT}/WDL
)



