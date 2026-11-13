// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef FS_INTERNAL_H_
#define FS_INTERNAL_H_
#include "filesystem_core.h"
extern fs_block_device_t fs_device;
extern unsigned fs_bound, fs_mounted, fs_files;
static inline int fs_handle_valid(unsigned handle) { return handle < FS_MAX_CLIENTS; }
static inline int fs_is_open(unsigned handle) { return fs_handle_valid(handle) && (fs_files & (1u << handle)); }
fs_result_t fs_validate_open(unsigned handle, const char *path, unsigned mode);
fs_result_t fs_normalize_path(const char *path, char output[FS_MAX_PATH]);
#endif
