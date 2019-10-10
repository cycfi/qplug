###############################################################################
#  Copyright (c) 2016-2019 Joel de Guzman
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################

project(presets_test)

add_executable(presets_test presets_test.cpp)

target_include_directories(presets_test
   PUBLIC
   ${QPLUG_INCLUDE_DIRS}
)

target_link_libraries(presets_test libelements libq infra)

