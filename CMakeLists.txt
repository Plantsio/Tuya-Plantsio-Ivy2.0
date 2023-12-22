# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

list(APPEND EXTRA_COMPONENT_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/tuya
        ${CMAKE_CURRENT_SOURCE_DIR}/ivy
        #${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/arduino
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
        )


set(IDF_TARGET "esp32s3")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(firmware)