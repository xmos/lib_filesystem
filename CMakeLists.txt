cmake_minimum_required(VERSION 3.21)
project(filesystem_native C)
include(CTest)
set(CMAKE_C_STANDARD 11)
if(DEFINED FS_BACKEND)
    message(FATAL_ERROR "FS_BACKEND was removed: native tests now exercise both header-selected backends; clear FS_BACKEND from the CMake cache")
endif()
set(FATFS_REPO "${CMAKE_CURRENT_SOURCE_DIR}/../lib_fatfs" CACHE PATH "Pinned lib_fatfs checkout")
set(fatfs_source "${FATFS_REPO}/lib_fatfs/src/ff_wrapper.c")
if(NOT EXISTS "${fatfs_source}")
    message(FATAL_ERROR "Provide a lib_fatfs checkout via FATFS_REPO")
endif()
option(FS_SANITIZE "Enable native address/undefined sanitizers" ${BUILD_TESTING})
add_compile_definitions(_POSIX_C_SOURCE=200809L)
if(FS_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(fatfs_native STATIC "${fatfs_source}")
target_include_directories(fatfs_native PUBLIC "${FATFS_REPO}/lib_fatfs/api")
set(core lib_filesystem/src/fs_common.c
    lib_filesystem/src/filesystems/fatfs/fs_fatfs.c
    lib_filesystem/src/filesystems/fatfs/fs_fatfs_disk.c
    lib_filesystem/src/filesystems/petit/fs_petit.c
    lib_filesystem/src/filesystems/petit/fs_petit_disk.c
    lib_filesystem/src/filesystems/petit/petit_wrapper.c)

# All regressions use the same source list. Only the optional application
# header selects a backend/client limit, not a CMake option.
add_library(filesystem_core STATIC ${core})
add_library(filesystem_core_multiclient STATIC ${core})
add_library(filesystem_core_petit STATIC ${core})
target_include_directories(filesystem_core_multiclient PUBLIC tests/config/fatfs)
target_include_directories(filesystem_core_petit PUBLIC tests/config/petit)
foreach(target filesystem_core filesystem_core_multiclient filesystem_core_petit)
    target_include_directories(${target} PUBLIC lib_filesystem/api PRIVATE lib_filesystem/src)
    target_link_libraries(${target} PRIVATE fatfs_native)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
endforeach()

# Host formatting is always full FatFs, independently of target configuration.
add_executable(fat_image_creator lib_filesystem/disk_image_creators/fat/src/make_fat.c
    tools/image_device.c lib_filesystem/src/fs_common.c
    lib_filesystem/src/filesystems/fatfs/fs_fatfs_disk.c)
target_include_directories(fat_image_creator PRIVATE tools lib_filesystem/api lib_filesystem/src)
target_link_libraries(fat_image_creator PRIVATE fatfs_native)
target_compile_options(fat_image_creator PRIVATE -Wall -Wextra -Werror)

if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    foreach(backend fatfs fatfs_multiclient petit)
        if(backend STREQUAL "fatfs")
            set(core_target filesystem_core)
        elseif(backend STREQUAL "fatfs_multiclient")
            set(core_target filesystem_core_multiclient)
        else()
            set(core_target filesystem_core_petit)
        endif()
        add_executable(filesystem_core_test_${backend} tests/core_test.c tools/image_device.c)
        target_include_directories(filesystem_core_test_${backend} PRIVATE tools)
        target_link_libraries(filesystem_core_test_${backend} PRIVATE ${core_target})
        target_compile_options(filesystem_core_test_${backend} PRIVATE -Wall -Wextra -Werror)
        add_test(NAME filesystem_native_${backend}
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/native_test.py
            --creator $<TARGET_FILE:fat_image_creator> --test $<TARGET_FILE:filesystem_core_test_${backend}>
            --work ${CMAKE_CURRENT_BINARY_DIR}/fixtures-${backend})
        set_tests_properties(filesystem_native_${backend} PROPERTIES TIMEOUT 90
            ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")
    endforeach()
    add_test(NAME filesystem_configuration COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/config_test.py --compiler ${CMAKE_C_COMPILER}
        --api ${CMAKE_CURRENT_SOURCE_DIR}/lib_filesystem/api)
    add_test(NAME filesystem_link_selection COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/link_test.py --tool ${CMAKE_NM}
        --fatfs $<TARGET_FILE:filesystem_core_test_fatfs> --petit $<TARGET_FILE:filesystem_core_test_petit>)
    set_tests_properties(filesystem_configuration filesystem_link_selection PROPERTIES TIMEOUT 90)
endif()
