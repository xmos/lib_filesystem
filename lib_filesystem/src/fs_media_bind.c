// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem_core.h"
extern fs_media_status_t fs_media_initialise_xc(void);
extern fs_media_status_t fs_media_status_xc(void);
extern fs_result_t fs_media_read_xc(uint8_t *buffer, uint32_t sector, uint32_t offset, uint32_t count);
extern fs_result_t fs_media_write_xc(const uint8_t *buffer, uint32_t sector, uint32_t count);
extern fs_result_t fs_media_ioctl_xc(uint8_t command, void *buffer);
static FS_MEDIA_CALLBACK(initialise) fs_media_status_t initialise(void *context) { (void)context; return fs_media_initialise_xc(); }
static FS_MEDIA_CALLBACK(status) fs_media_status_t status(void *context) { (void)context; return fs_media_status_xc(); }
static FS_MEDIA_CALLBACK(read) fs_result_t read(void *context, uint8_t *buffer, uint32_t sector, uint32_t offset, uint32_t count)
{ (void)context; return fs_media_read_xc(buffer, sector, offset, count); }
static FS_MEDIA_CALLBACK(write) fs_result_t write(void *context, const uint8_t *buffer, uint32_t sector, uint32_t count)
{ (void)context; return fs_media_write_xc(buffer, sector, count); }
static FS_MEDIA_CALLBACK(ioctl) fs_result_t ioctl(void *context, uint8_t command, void *buffer)
{ (void)context; return fs_media_ioctl_xc(command, buffer); }
fs_result_t fs_bind_service(void)
{
    static unsigned claimed;
    if (claimed) return FS_RES_BUSY;
    fs_block_device_t device = {NULL, initialise, status, read, write, ioctl};
    fs_result_t rc = fs_bind(&device);
    if (!rc) claimed = 1;
    return rc;
}
