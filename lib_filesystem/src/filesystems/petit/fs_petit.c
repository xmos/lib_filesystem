// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem_config.h"
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_PETIT
#include <limits.h>
#include "fs_internal.h"
#include "petit_config.h"
#include "../../../../third_party/PetitFatFs/source/pff.h"
static FATFS volume;
void fs_petit_detach(void);
const char *fs_backend_name(void) { return "Petit FatFs"; }
fs_result_t fs_mount(fs_format_t format)
{
    if ((unsigned)format > FS_FORMAT_AUTO) return FS_RES_INVALID;
    if (!fs_bound) return FS_RES_NOT_ENABLED;
    if (fs_mounted) return format == FS_FORMAT_AUTO || volume.fs_type == (unsigned)format + 1 ? FS_RES_OK : FS_RES_NO_FILESYSTEM;
    FRESULT rc = pf_mount(&volume);
    if (rc) { fs_petit_detach(); fs_mounted = 0; return (fs_result_t)rc; }
    if (format != FS_FORMAT_AUTO && volume.fs_type != (unsigned)format + 1) {
        fs_petit_detach(); fs_mounted = 0; return FS_RES_NO_FILESYSTEM;
    }
    fs_mounted = 1; return FS_RES_OK;
}
fs_result_t fs_unmount(void)
{
    if (fs_files) return FS_RES_BUSY;
    fs_petit_detach();
    fs_mounted = 0;
    return FS_RES_OK;
}
fs_result_t fs_open(unsigned handle, const char *path, unsigned mode)
{
    fs_result_t rc = fs_validate_open(handle, path, mode);
    if (rc) return rc;
    if (mode != FS_OPEN_READ) return FS_RES_UNSUPPORTED;
    if (fs_files) return FS_RES_BUSY; /* Petit has one file, not one per client. */
    char canonical[FS_MAX_PATH];
    rc = fs_normalize_path(path, canonical);
    if (rc) return rc;
    FRESULT prc = pf_open(canonical);
    if (!prc) fs_files = 1u << handle;
    return (fs_result_t)prc;
}
fs_result_t fs_close(unsigned handle)
{
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    volume.flag = 0;
    fs_files = 0;
    return FS_RES_OK;
}
fs_result_t fs_read(unsigned handle, void *buffer, size_t count, size_t *read)
{
    if (!read) return FS_RES_INVALID;
    *read = 0;
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    if ((!buffer && count) || count > UINT_MAX) return FS_RES_INVALID;
    if (!count) return FS_RES_OK;
    UINT actual = 0;
    FRESULT rc = pf_read(buffer, count, &actual);
    *read = actual;
    return (fs_result_t)rc;
}
fs_result_t fs_write(unsigned handle, const void *buffer, size_t count, size_t *written)
{
    (void)handle; (void)buffer; (void)count;
    if (written) *written = 0;
    return written ? FS_RES_UNSUPPORTED : FS_RES_INVALID;
}
fs_result_t fs_seek(unsigned handle, size_t offset, int from_start)
{
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    size_t base = from_start ? 0 : volume.fptr;
    if (offset > UINT32_MAX - base || base + offset > volume.fsize) return FS_RES_INVALID;
    return (fs_result_t)pf_lseek(base + offset);
}
fs_result_t fs_size(unsigned handle, size_t *size)
{
    if (!size) return FS_RES_INVALID;
    *size = 0;
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    *size = volume.fsize; return FS_RES_OK;
}
fs_result_t fs_tell(unsigned handle, size_t *offset)
{
    if (!offset) return FS_RES_INVALID;
    *offset = 0;
    if (!fs_is_open(handle)) return FS_RES_NOT_OPENED;
    *offset = volume.fptr; return FS_RES_OK;
}
fs_result_t fs_sync(unsigned handle) { return fs_is_open(handle) ? FS_RES_OK : FS_RES_NOT_OPENED; }
#endif
