# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
#set(APP_BIN_NAME $ENV{APP_BIN_NAME})
#set(USER_SW_VER $ENV{USER_SW_VER})
#set(APP_VERSION $ENV{APP_VERSION})
#set(APP_PATH $ENV{APP_PATH})
list(APPEND EXTRA_COMPONENT_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/tuya
        #${CMAKE_CURRENT_SOURCE_DIR}/ivy
        #${CMAKE_CURRENT_SOURCE_DIR}/app
        ${CMAKE_CURRENT_SOURCE_DIR}/arduino
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
        )


set(IDF_TARGET "esp32s3")
#set(PROJECT_NAME $ENV{APP_BIN_NAME})
#message(WARNING "EXTRA_COMPONENT_DIRS=${EXTRA_COMPONENT_DIRS}")
#message(WARNING "APP_BIN_NAME=$ENV{APP_BIN_NAME}")
#message(WARNING "IDF_PATH=$IDF_PATH")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(firmware)