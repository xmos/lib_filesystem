// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef FS_PETIT_CONFIG_H_
#define FS_PETIT_CONFIG_H_
/* Predefine the upstream configuration guard so its default pffconf.h is not
 * edited. This is the documented read/seek, single-file, 8.3-name subset. */
#define PFCONF_DEF 8088
#define PF_USE_READ 1
#define PF_USE_DIR 0
#define PF_USE_LSEEK 1
#define PF_USE_WRITE 0
#define PF_FS_FAT12 1
#define PF_FS_FAT16 1
#define PF_FS_FAT32 1
#define PF_USE_LCC 1
#define PF_CODE_PAGE 437
/* Full FatFs is a build dependency even when unused. Keep Petit's disk ABI
 * private: its zero-argument disk_initialize differs from full FatFs's. */
#define disk_initialize fs_petit_disk_initialize
#define disk_readp fs_petit_disk_readp
#define disk_writep fs_petit_disk_writep
#endif
