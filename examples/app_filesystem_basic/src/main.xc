// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include "example.h"

int main(void)
{
    interface fs_basic_if files[1];
    interface fs_block_media_if media;
    par {
        on tile[0]: ram_media(media);
        /* Exactly one filesystem owner. Neither other task calls FatFs. */
        on tile[1]: filesystem_basic_block(files, 1, FS_FORMAT_AUTO, media);
        on tile[0]: read_hello(files[0]);
    }
    return 0;
}
