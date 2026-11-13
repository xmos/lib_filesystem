// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem_config.h"
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_PETIT
#include "petit_config.h"
#include <stddef.h>
#include "../../../../third_party/PetitFatFs/source/pff.c"

/* At the pinned R0.03a, pf_mount(NULL) dereferences NULL after reading the
 * volume, despite the header's mount/unmount comment. Detach inside this TU
 * instead; keep upstream unchanged and never issue I/O during unmount. */
void fs_petit_detach(void) { FatFs = NULL; }
#endif
