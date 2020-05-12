###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

project(presets_test)

###############################################################################
# Boost

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.61 REQUIRED)

###############################################################################
add_executable(presets_test presets_test.cpp)
include_directories(${Boost_INCLUDE_DIRS})

target_include_directories(presets_test
   PUBLIC
   ${QPLUG_INCLUDE_DIRS}
   ${BOOST_LIBRARYDIR}
   ../lib/infra/include
)

target_link_libraries(presets_test libq)

