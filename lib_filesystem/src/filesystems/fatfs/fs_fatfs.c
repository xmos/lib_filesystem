// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem_config.h"
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_FATFS
#include <limits.h>
#include <string.h>
#include "fs_internal.h"
#include "ff.h"
static FATFS volume;
static FIL files[FS_MAX_CLIENTS];
static char paths[FS_MAX_CLIENTS][FS_MAX_PATH];

static fs_result_t result(FRESULT rc)
{
    switch (rc) {
    case FR_OK: return FS_RES_OK;
    case FR_DISK_ERR: case FR_INT_ERR: return FS_RES_DISK_ERR;
    case FR_NOT_READY: return FS_RES_NOT_READY;
    case FR_NO_FILE: case FR_NO_PATH: return FS_RES_NO_FILE;
    case FR_INVALID_OBJECT: return FS_RES_NOT_OPENED;
    case FR_NOT_ENABLED: return FS_RES_NOT_ENABLED;
    case FR_NO_FILESYSTEM: return FS_RES_NO_FILESYSTEM;
    case FR_DENIED: case FR_WRITE_PROTECTED: return FS_RES_DENIED;
    case FR_EXIST: return FS_RES_EXISTS;
    case FR_LOCKED: case FR_TIMEOUT: return FS_RES_BUSY;
    default: return FS_RES_INVALID;
    }
}
const char *fs_backend_name(void) { return "FatFs"; }
fs_result_t fs_mount(fs_format_t format)
{
    if ((unsigned)format > FS_FORMAT_AUTO) return FS_RES_INVALID;
    if (!fs_bound) return FS_RES_NOT_ENABLED;
    if (fs_mounted) return format == FS_FORMAT_AUTO || volume.fs_type == (unsigned)format + 1 ? FS_RES_OK : FS_RES_NO_FILESYSTEM;
    FRESULT rc = f_mount(&volume, "0:", 1);
    if (rc != FR_OK) { f_mount(NULL, "0:", 0); return result(rc); }
    if (format != FS_FORMAT_AUTO && volume.fs_type != (unsigned)format + 1) {
        f_mount(NULL, "0:", 0); return FS_RES_NO_FILESYSTEM;
    }
    fs_mounted = 1;
    return FS_RES_OK;
}
fs_result_t fs_unmount(void)
{
    if (fs_files) return FS_RES_BUSY;
    FRESULT rc = f_mount(NULL, "0:", 0);
    if (rc == FR_OK) fs_mounted = 0;
    return result(rc);
}
fs_result_t fs_open(unsigned handle, const char *path, unsigned mode)
{
    fs_result_t check = fs_validate_open(handle, path, mode);
    if (check) return check;
    char canonical[FS_MAX_PATH];
    check = fs_normalize_path(path, canonical);
    if (check) return check;
    for (unsigned other = 0; other < FS_MAX_CLIENTS; ++other)
        if (fs_is_open(other) && !strcmp(paths[other], canonical)) return FS_RES_BUSY;
    unsigned flags = 0;
    if (mode & FS_OPEN_READ) flags |= FA_READ;
    if (mode & FS_OPEN_WRITE) flags |= FA_WRITE;
    if (mode & FS_OPEN_CREATE_NEW) flags |= FA_CREATE_NEW;
    if (mode & FS_OPEN_CREATE_ALWAYS) flags |= FA_CREATE_ALWAYS;
    FRESULT rc = f_open(&files[handle], canonical, flags);
    if (!rc) { fs_files |= 1u << handle; strcpy(paths[handle], canonical); }
    return result(rc);
}
fs_result_t fs_close(unsigned handle)
{
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    FRESULT rc = f_close(&files[handle]);
    /* Release the slot even on error. FatFs invalidates open files when the
     * media is re-initialised, and a handle that could never be closed would
     * keep this client, its path and unmount busy until reboot. */
    fs_files &= ~(1u << handle);
    return result(rc);
}
fs_result_t fs_read(unsigned handle, void *buffer, size_t count, size_t *read)
{
    if (!read) return FS_RES_INVALID;
    *read = 0;
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    if ((!buffer && count) || count > UINT_MAX) return FS_RES_INVALID;
    if (!count) return FS_RES_OK;
    UINT actual = 0;
    FRESULT rc = f_read(&files[handle], buffer, count, &actual);
    *read = actual;
    return result(rc);
}
fs_result_t fs_write(unsigned handle, const void *buffer, size_t count, size_t *written)
{
    if (!written) return FS_RES_INVALID;
    *written = 0;
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    if ((!buffer && count) || count > UINT_MAX) return FS_RES_INVALID;
    if (!count) return FS_RES_OK;
    UINT actual = 0;
    FRESULT rc = f_write(&files[handle], buffer, count, &actual);
    *written = actual;
    return rc == FR_OK && actual != count ? FS_RES_FULL : result(rc);
}
fs_result_t fs_seek(unsigned handle, size_t offset, int from_start)
{
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    size_t base = from_start ? 0 : f_tell(&files[handle]);
    if (offset > UINT32_MAX - base) return FS_RES_INVALID;
    FRESULT rc = f_lseek(&files[handle], base + offset);
    if (!rc && f_tell(&files[handle]) != base + offset) return FS_RES_INVALID;
    return result(rc);
}
fs_result_t fs_size(unsigned handle, size_t *size)
{
    if (!size) return FS_RES_INVALID;
    *size = 0;
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    *size = f_size(&files[handle]); return FS_RES_OK;
}
fs_result_t fs_tell(unsigned handle, size_t *offset)
{
    if (!offset) return FS_RES_INVALID;
    *offset = 0;
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    *offset = f_tell(&files[handle]); return FS_RES_OK;
}
fs_result_t fs_sync(unsigned handle)
{
    return fs_is_open(handle) ? result(f_sync(&files[handle])) : FS_RES_NOT_OPENED;
}
#endif
