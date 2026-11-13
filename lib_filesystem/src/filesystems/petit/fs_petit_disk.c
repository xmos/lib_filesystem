// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem_config.h"
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_PETIT
#include "fs_internal.h"
#include "petit_config.h"
#include "../../../../third_party/PetitFatFs/source/diskio.h"
DSTATUS disk_initialize(void)
{
    if (!fs_bound) return STA_NOINIT;
    fs_media_status_t status = fs_device.initialise(fs_device.context);
    return status == FS_MED_OK || status == FS_MED_WRITE_PROTECTED ? 0 : STA_NOINIT;
}
DRESULT disk_readp(BYTE *buffer, DWORD sector, UINT offset, UINT count)
{
    if (!fs_bound || !buffer || offset > 512 || count > 512 - offset) return RES_PARERR;
    return fs_device.read(fs_device.context, buffer, sector, offset, count) == FS_RES_OK ? RES_OK : RES_ERROR;
}
DRESULT disk_writep(const BYTE *buffer, DWORD count)
{
    (void)buffer; (void)count;
    return RES_PARERR; /* The selected Petit configuration is read-only. */
}
#endif
