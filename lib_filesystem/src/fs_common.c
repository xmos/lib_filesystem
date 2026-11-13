// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "fs_internal.h"
fs_block_device_t fs_device;
unsigned fs_bound, fs_mounted, fs_files;

fs_result_t fs_bind(const fs_block_device_t *device)
{
    if (!device || !device->initialise || !device->status || !device->read || !device->ioctl)
        return FS_RES_INVALID;
    if (fs_mounted || fs_files) return FS_RES_BUSY;
    fs_device = *device;
    fs_bound = 1;
    return FS_RES_OK;
}

fs_result_t fs_validate_open(unsigned handle, const char *path, unsigned mode)
{
    if (!fs_handle_valid(handle) || !path || !mode ||
        (mode & ~(FS_OPEN_READ | FS_OPEN_WRITE | FS_OPEN_CREATE_NEW | FS_OPEN_CREATE_ALWAYS)) ||
        !(mode & (FS_OPEN_READ | FS_OPEN_WRITE)) ||
        ((mode & (FS_OPEN_CREATE_NEW | FS_OPEN_CREATE_ALWAYS)) && !(mode & FS_OPEN_WRITE)) ||
        ((mode & FS_OPEN_CREATE_NEW) && (mode & FS_OPEN_CREATE_ALWAYS))) return FS_RES_INVALID;
    if (!fs_mounted) return FS_RES_NOT_READY;
    if (fs_is_open(handle)) return FS_RES_BUSY;
    for (unsigned i = 0; i < FS_MAX_PATH; ++i) {
        if (path[i] == ':') return FS_RES_INVALID; /* One mounted volume, no drive switching. */
        if (!path[i]) return i ? FS_RES_OK : FS_RES_INVALID;
    }
    return FS_RES_INVALID;
}

fs_result_t fs_normalize_path(const char *path, char output[FS_MAX_PATH])
{
    unsigned done = 0, name = 0, extension = 0, dot = 0;
    for (unsigned i = 0; i < FS_MAX_PATH; ++i) {
        unsigned char c = path[i];
        if (!c || c == '/') {
            if (!name) {
                if (!c) return FS_RES_INVALID;
                continue; /* Leading or repeated separators. */
            }
            if (dot && !extension) return FS_RES_INVALID;
            if (!c) { output[done] = 0; return FS_RES_OK; }
            if (done + 1 >= FS_MAX_PATH) return FS_RES_INVALID;
            output[done++] = '/'; name = extension = dot = 0;
            continue;
        }
        if (c < 33 || c > 126 || c == ':' || c == '\\' || c == '"' || c == '*' ||
            c == '?' || c == '<' || c == '>' || c == '|' || c == '[' || c == ']' || c == ';' || c == ',')
            return FS_RES_INVALID;
        if (c == '.') {
            if (dot || !name) return FS_RES_INVALID;
            dot = 1;
        } else if (dot) {
            if (++extension > 3) return FS_RES_INVALID;
        } else if (++name > 8) return FS_RES_INVALID;
        if (done + 1 >= FS_MAX_PATH) return FS_RES_INVALID;
        output[done++] = c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
    }
    return FS_RES_INVALID;
}
