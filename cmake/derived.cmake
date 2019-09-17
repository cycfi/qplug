###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.5.1)

set(BUNDLE_NAME "${PLUG_NAME}")

if (NOT DEFINED PLUG_CONTROLLER)
   set(PLUG_CONTROLLER "${PLUG_NAME}_controller")
endif()

if (NOT DEFINED PLUG_CONTROLLER_INCLUDE)
   set(PLUG_CONTROLLER_INCLUDE "${PLUG_CONTROLLER}.hpp")
endif()

if (NOT DEFINED PLUG_PROCESSOR)
   set(PLUG_PROCESSOR "${PLUG_NAME}_processor")
endif()

if (NOT DEFINED PLUG_PROCESSOR_INCLUDE)
   set(PLUG_PROCESSOR_INCLUDE "${PLUG_PROCESSOR}.hpp")
endif()

if (NOT DEFINED SHARED_RESOURCES_SUBPATH)
   set(SHARED_RESOURCES_SUBPATH "${PLUG_NAME}")
endif()

set(AUV2_ENTRY ${PLUG_NAME}_Entry)
set(AUV2_ENTRY_STR "${PLUG_NAME}_Entry")
set(AUV2_FACTORY ${PLUG_NAME}_Factory)
set(AUV2_VIEW_CLASS ${PLUG_NAME}_View)
set(AUV2_VIEW_CLASS_STR "${PLUG_NAME}_View")
set(AAX_PLUG_MFR_STR \"${PLUG_MFR_ID}\")
set(AAX_PLUG_NAME_STR "${PLUG_NAME}\\n${PLUG_UNIQUE_ID}")

