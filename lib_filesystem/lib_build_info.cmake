set(LIB_NAME lib_filesystem)
set(LIB_VERSION 0.1.0)
if(DEFINED FS_BACKEND)
    message(FATAL_ERROR "FS_BACKEND was removed: use filesystem_conf.h and clear FS_BACKEND from the CMake cache")
endif()
set(LIB_INCLUDES api src)
set(LIB_OPTIONAL_HEADERS filesystem_conf.h)
set(LIB_COMPILER_FLAGS -Os -g)
# Resolve full FatFs for the default configuration. In a Petit application no
# f_* entry points are referenced, so its code and state are not linked.
set(LIB_DEPENDENT_MODULES "lib_xassert(4.3.2)"
    "https://github.com/xross/lib_fatfs(cc154623a90b710caa2a80c8955600f9e00b61dc)")
set(LIB_C_SRCS src/fs_common.c src/fs_media_bind.c
    src/filesystems/fatfs/fs_fatfs.c src/filesystems/fatfs/fs_fatfs_disk.c
    src/filesystems/petit/fs_petit.c src/filesystems/petit/fs_petit_disk.c
    src/filesystems/petit/petit_wrapper.c)
set(LIB_XC_SRCS src/filesystem.xc src/fs_media.xc src/storage_bridge.xc)
set(LIB_CXX_SRCS "")
set(LIB_ASM_SRCS "")
option(FS_ENABLE_QSPI "Build the optional quadflash storage adapter" OFF)
if(FS_ENABLE_QSPI)
    list(APPEND LIB_XC_SRCS src/storage_media/qspi_flash.xc)
    foreach(target ${APP_BUILD_TARGETS})
        target_link_libraries(${target} PRIVATE quadflash)
    endforeach()
endif()
XMOS_REGISTER_MODULE()
