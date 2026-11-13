cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(app_filesystem_basic)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(APP_HW_TARGET src/XK-EVK-XU316-AIV.xn CACHE STRING "Example hardware target")
set(APP_INCLUDES src ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(APP_COMPILER_FLAGS -O2 -g -report -fxscope)
set(APP_XSCOPE_SRCS config.xscope)
include(${CMAKE_CURRENT_LIST_DIR}/../../deps.cmake)

# Use a separately built host tool, never a target executable, to prepare media.
set(host_suffix "")
if(CMAKE_HOST_WIN32)
    set(host_suffix .exe)
endif()
set(FAT_IMAGE_CREATOR "${CMAKE_CURRENT_LIST_DIR}/../../build-native/fat_image_creator${host_suffix}"
    CACHE FILEPATH "Native CMake fat_image_creator executable")
if(NOT EXISTS "${FAT_IMAGE_CREATOR}")
    message(FATAL_ERROR "Build the native fat_image_creator first; see the Basic example section in doc/rst/lib_filesystem.rst")
endif()
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)
execute_process(COMMAND "${FAT_IMAGE_CREATOR}" --force --c-header --sectors 128
    "${CMAKE_CURRENT_BINARY_DIR}/generated/image.h" "${CMAKE_CURRENT_LIST_DIR}/data/HELLO.TXT"
    COMMAND_ERROR_IS_FATAL ANY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${FAT_IMAGE_CREATOR}" "${CMAKE_CURRENT_LIST_DIR}/data/HELLO.TXT")

XMOS_REGISTER_APP()
set_target_properties(${APP_BUILD_TARGETS} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
