// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef FILESYSTEM_EXAMPLE_H_
#define FILESYSTEM_EXAMPLE_H_
#include "filesystem.h"

void read_hello(client interface fs_basic_if files);
[[distributable]]
void ram_media(server interface fs_block_media_if media);

#endif
