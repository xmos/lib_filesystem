// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef FS_IMAGE_DEVICE_H_
#define FS_IMAGE_DEVICE_H_
#include <stdio.h>
#include "filesystem_core.h"
typedef struct { FILE *file; uint32_t sectors; int readonly, fail_read, fail_write; } fs_image_t;
fs_block_device_t fs_image_device(fs_image_t *image);
#endif
