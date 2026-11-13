// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem_config.h"
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_FATFS
#include "fs_internal.h"
#include "diskio.h"

static DSTATUS status(fs_media_status_t value)
{
    switch (value) {
    case FS_MED_OK: return 0;
    case FS_MED_WRITE_PROTECTED: return STA_PROTECT;
    case FS_MED_NO_DISK: return STA_NODISK | STA_NOINIT;
    default: return STA_NOINIT;
    }
}
DSTATUS disk_initialize(BYTE drive) { return drive || !fs_bound ? STA_NOINIT : status(fs_device.initialise(fs_device.context)); }
DSTATUS disk_status(BYTE drive) { return drive || !fs_bound ? STA_NOINIT : status(fs_device.status(fs_device.context)); }
DRESULT disk_read(BYTE drive, BYTE *buffer, LBA_t sector, UINT count)
{
    if (drive || !fs_bound || !count || count > UINT32_MAX / 512) return RES_PARERR;
    return fs_device.read(fs_device.context, buffer, sector, 0, count * 512) == FS_RES_OK ? RES_OK : RES_ERROR;
}
DRESULT disk_write(BYTE drive, const BYTE *buffer, LBA_t sector, UINT count)
{
    if (drive || !fs_bound || !count || count > UINT32_MAX / 512) return RES_PARERR;
    if (!fs_device.write) return RES_WRPRT;
    return fs_device.write(fs_device.context, buffer, sector, count * 512) == FS_RES_OK ? RES_OK : RES_ERROR;
}
DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer)
{
    if (drive || !fs_bound) return RES_PARERR;
    return fs_device.ioctl(fs_device.context, command, buffer) == FS_RES_OK ? RES_OK : RES_ERROR;
}
/* No target RTC policy is imposed. Deterministic default: 2026-01-01. */
DWORD get_fattime(void) { return (46u << 25) | (1u << 21) | (1u << 16); }
#endif
