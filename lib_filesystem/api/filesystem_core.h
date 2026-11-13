// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef FILESYSTEM_CORE_H_
#define FILESYSTEM_CORE_H_
#include "filesystem.h"
#ifdef __XC__
#define FS_UNSAFE unsafe
#else
#define FS_UNSAFE
#endif
#ifdef __xcore__
#define FS_MEDIA_CALLBACK(name) __attribute__((fptrgroup("fs_media_" #name)))
#else
#define FS_MEDIA_CALLBACK(name)
#endif
#ifdef __cplusplus
extern "C" {
#endif
#ifndef __XC__
typedef struct {
    void *context;
    FS_MEDIA_CALLBACK(initialise) fs_media_status_t (*initialise)(void *context);
    FS_MEDIA_CALLBACK(status) fs_media_status_t (*status)(void *context);
    FS_MEDIA_CALLBACK(read) fs_result_t (*read)(void *context, uint8_t *buffer, uint32_t sector, uint32_t offset, uint32_t count);
    FS_MEDIA_CALLBACK(write) fs_result_t (*write)(void *context, const uint8_t *buffer, uint32_t sector, uint32_t count);
    FS_MEDIA_CALLBACK(ioctl) fs_result_t (*ioctl)(void *context, uint8_t command, void *buffer);
} fs_block_device_t;
/* Optional portable C entry points, owned by one task. No heap, callbacks
 * supplied by storage only. Handle is the client index [0,FS_MAX_CLIENTS). */
fs_result_t fs_bind(const fs_block_device_t *device);
#endif
fs_result_t fs_mount(fs_format_t format);
fs_result_t fs_unmount(void);
fs_result_t fs_open(unsigned handle, const char * FS_UNSAFE path, unsigned mode);
fs_result_t fs_close(unsigned handle);
fs_result_t fs_read(unsigned handle, void * FS_UNSAFE buffer, size_t count, size_t * FS_UNSAFE read);
fs_result_t fs_write(unsigned handle, const void * FS_UNSAFE buffer, size_t count, size_t * FS_UNSAFE written);
fs_result_t fs_seek(unsigned handle, size_t offset, int from_start);
fs_result_t fs_size(unsigned handle, size_t * FS_UNSAFE size);
fs_result_t fs_tell(unsigned handle, size_t * FS_UNSAFE offset);
fs_result_t fs_sync(unsigned handle);
#ifndef __XC__
const char *fs_backend_name(void);
#endif
#ifdef __cplusplus
}
#endif
#endif
