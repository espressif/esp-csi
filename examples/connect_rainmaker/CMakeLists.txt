# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
add_compile_options(-fdiagnostics-color=always)

if(DEFINED ENV{RMAKER_PATH})
  set(RMAKER_PATH $ENV{RMAKER_PATH})
else()
  set(RMAKER_PATH ${CMAKE_CURRENT_LIST_DIR}/components/esp-rainmaker)
endif(DEFINED ENV{RMAKER_PATH})

set(CSI_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)

set(EXTRA_COMPONENT_DIRS ${CSI_PATH}
                         ${RMAKER_PATH}/components/esp-insights/components 
                         ${RMAKER_PATH}/components
                         ${RMAKER_PATH}/components/esp-rainmaker/components
                         ${RMAKER_PATH}/examples/common/app_reset
                         ${RMAKER_PATH}/examples/common/app_wifi
                         ${RMAKER_PATH}/examples/common/app_insights
                         )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
string(REGEX REPLACE ".*/\(.*\)" "\\1" CURDIR ${CMAKE_CURRENT_SOURCE_DIR})
project(${CURDIR})

git_describe(PROJECT_VERSION ${COMPONENT_DIR})
message("Project commit: " ${PROJECT_VERSION})
