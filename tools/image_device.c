// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "image_device.h"
#include <stdint.h>
static fs_media_status_t initial(void *context)
{
    fs_image_t *image = context;
    return !image->file ? FS_MED_NOT_INIT : image->readonly ? FS_MED_WRITE_PROTECTED : FS_MED_OK;
}
static fs_result_t read_image(void *context, uint8_t *buffer, uint32_t sector, uint32_t offset, uint32_t count)
{
    fs_image_t *image = context;
    uint64_t position = (uint64_t)sector * 512 + offset, bytes = (uint64_t)image->sectors * 512;
    if (!image->file) return FS_RES_NOT_READY;
    if (position > bytes || count > bytes - position || offset >= 512) return FS_RES_INVALID;
    if (image->fail_read || fseeko(image->file, (off_t)position, SEEK_SET)) return FS_RES_DISK_ERR;
    return fread(buffer, 1, count, image->file) == count ? FS_RES_OK : FS_RES_DISK_ERR;
}
static fs_result_t write_image(void *context, const uint8_t *buffer, uint32_t sector, uint32_t count)
{
    fs_image_t *image = context;
    uint64_t position = (uint64_t)sector * 512, bytes = (uint64_t)image->sectors * 512;
    if (!image->file) return FS_RES_NOT_READY;
    if (position > bytes || count > bytes - position) return FS_RES_INVALID;
    if (image->readonly) return FS_RES_DENIED;
    if (image->fail_write || fseeko(image->file, (off_t)position, SEEK_SET)) return FS_RES_DISK_ERR;
    return fwrite(buffer, 1, count, image->file) == count ? FS_RES_OK : FS_RES_DISK_ERR;
}
static fs_result_t ioctl_image(void *context, uint8_t command, void *buffer)
{
    fs_image_t *image = context;
    if (!image->file) return FS_RES_NOT_READY;
    switch (command) {
    case FS_CTRL_SYNC: return fflush(image->file) ? FS_RES_DISK_ERR : FS_RES_OK;
    case FS_GET_SECTOR_COUNT: if (!buffer) return FS_RES_INVALID; *(uint32_t *)buffer = image->sectors; break;
    case FS_GET_SECTOR_SIZE: if (!buffer) return FS_RES_INVALID; *(uint16_t *)buffer = 512; break;
    case FS_GET_BLOCK_SIZE: if (!buffer) return FS_RES_INVALID; *(uint32_t *)buffer = 1; break;
    default: return FS_RES_UNSUPPORTED;
    }
    return FS_RES_OK;
}
fs_block_device_t fs_image_device(fs_image_t *image)
{
    fs_block_device_t result = {image, initial, initial, read_image, write_image, ioctl_image};
    return result;
}
