# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
add_compile_options(-fdiagnostics-color=always)

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/components/csi)
    set(CSI_PATH ${CMAKE_CURRENT_LIST_DIR}/components/csi)
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/components/esp-csi)
    set(CSI_PATH ${CMAKE_CURRENT_LIST_DIR}/components/esp-csi)
elseif(EXISTS $ENV{IDF_PATH}/components/csi)
    set(CSI_PATH $ENV{IDF_PATH}/components/csi)
elseif(EXISTS $ENV{IDF_PATH}/components/esp-csi)
    set(CSI_PATH $ENV{IDF_PATH}/components/esp-csi)
else()
    set(CSI_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)
endif()

set(EXTRA_COMPONENT_DIRS ${CSI_PATH} $ENV{IDF_PATH}/examples/system/console/components
                         $ENV{IDF_PATH}/examples/system/console/advanced/components
                         $ENV{IDF_PATH}/examples/peripherals/rmt/led_strip/components
                         $ENV{IDF_PATH}/examples/common_components/led_strip
                         )


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
string(REGEX REPLACE ".*/\(.*\)" "\\1" CURDIR ${CMAKE_CURRENT_SOURCE_DIR})
project(${CURDIR})

git_describe(PROJECT_VERSION ${COMPONENT_DIR})
message("Project commit: " ${PROJECT_VERSION})
