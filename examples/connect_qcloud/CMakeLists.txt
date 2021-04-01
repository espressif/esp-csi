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


if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../../config/)
    set(QCLOUD_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/components/qcloud)
    set(QCLOUD_PATH ${CMAKE_CURRENT_LIST_DIR}/components/qcloud)
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/components/esp-qcloud)
    set(QCLOUD_PATH ${CMAKE_CURRENT_LIST_DIR}/components/esp-qcloud)
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/components/esp_qcloud)
    set(QCLOUD_PATH ${CMAKE_CURRENT_LIST_DIR}/components/esp_qcloud)
elseif(EXISTS $ENV{IDF_PATH}/components/qcloud)
    set(QCLOUD_PATH $ENV{IDF_PATH}/components/qcloud)
elseif(EXISTS $ENV{IDF_PATH}/components/esp-qcloud)
    set(QCLOUD_PATH $ENV{IDF_PATH}/components/esp-qcloud)
else()
    message(FATAL_ERROR "Please configure the path of `QCLOUD_PATH`")
endif()

if(NOT DEFINED IDF_TARGET)
    set(IDF_TARGET "esp32")
endif()

# Add QCloud components and other common application components
# This example uses an extra component for common functions such as led and button.
set(EXTRA_COMPONENT_DIRS ${QCLOUD_PATH} 
                         ${QCLOUD_PATH}/examples/common_components/led
                         ${QCLOUD_PATH}/examples/common_components/button
                         ${CSI_PATH} $ENV{IDF_PATH}/examples/system/console/components)

# Performance Options
if(NOT DEFINED PERF)
    set(PERF "defaults")
endif()



include($ENV{IDF_PATH}/tools/cmake/project.cmake)
string(REGEX REPLACE ".*/\(.*\)" "\\1" CURDIR ${CMAKE_CURRENT_SOURCE_DIR})
project(${CURDIR})

git_describe(PROJECT_VERSION ${COMPONENT_DIR})
message("Project commit: " ${PROJECT_VERSION})
