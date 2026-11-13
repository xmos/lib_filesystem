// Copyright 2015-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef __filesystem_h__
#define __filesystem_h__
#include "filesystem_config.h"
#include <stdint.h>
#include <stddef.h>

typedef enum { FS_FORMAT_FAT12, FS_FORMAT_FAT16, FS_FORMAT_FAT32, FS_FORMAT_AUTO } fs_format_t;
typedef enum { FS_MEDIA_SPI_FLASH, FS_MEDIA_QSPI_FLASH, FS_MEDIA_SDCARD, FS_MEDIA_USB } fs_media_t;
typedef enum { FS_MED_OK, FS_MED_NOT_INIT, FS_MED_NO_DISK, FS_MED_WRITE_PROTECTED } fs_media_status_t;
/* Preserve the existing result values; new errors are appended. */
typedef enum { FS_RES_OK, FS_RES_DISK_ERR, FS_RES_NOT_READY, FS_RES_NO_FILE,
               FS_RES_NOT_OPENED, FS_RES_NOT_ENABLED, FS_RES_NO_FILESYSTEM,
               FS_RES_INVALID, FS_RES_DENIED, FS_RES_EXISTS, FS_RES_BUSY,
               FS_RES_UNSUPPORTED, FS_RES_FULL } fs_result_t;
enum { FS_OPEN_READ = 1, FS_OPEN_WRITE = 2, FS_OPEN_CREATE_NEW = 4,
       FS_OPEN_CREATE_ALWAYS = 8, FS_MAX_CLIENTS = FILESYSTEM_MAX_CLIENTS, FS_MAX_PATH = 256,
       FS_TRANSFER_BYTES = 512 };
/* ioctl commands use the FatFs disk-I/O values. Payloads are uint32_t sector
 * count/block size and uint16_t sector size. Media must expose 512-byte sectors. */
enum { FS_CTRL_SYNC = 0, FS_GET_SECTOR_COUNT = 1, FS_GET_SECTOR_SIZE = 2, FS_GET_BLOCK_SIZE = 3 };

#ifdef __XC__
typedef interface fs_storage_media_if {
    fs_media_status_t initialise();
    fs_media_status_t status();
    /* count is bytes starting at sector*512+offset; reads may span sectors. */
    fs_result_t read(uint8_t *buf, uint32_t sector, uint32_t offset, uint32_t count);
    /* count is bytes starting at sector*512, not a sector count. */
    fs_result_t write(const uint8_t *buf, uint32_t sector, uint32_t count);
    fs_result_t ioctl(uint8_t cmd, uint8_t *buf);
} fs_storage_media_if;

/* Bounded alternative: safe across tiles. The old raw-pointer media interface
 * remains available for existing same-tile adapters. */
typedef interface fs_block_media_if {
    fs_media_status_t initialise();
    fs_media_status_t status();
    fs_result_t read_bytes(uint8_t buf[n], static const size_t n, uint32_t sector, uint32_t offset, uint32_t count);
    fs_result_t write_bytes(const uint8_t buf[n], static const size_t n, uint32_t sector, uint32_t count);
    fs_result_t control(uint8_t cmd, uint8_t buf[n], static const size_t n);
} fs_block_media_if;

/* Each client owns one file. All calls are serialised by this single owner
 * task. Do not run another service or direct FatFs calls concurrently. Paths
 * include a terminating NUL within n and are relative to the mounted root. */
typedef interface fs_basic_if {
    fs_result_t mount();
    fs_result_t open(char path[n], static const size_t n);
    fs_result_t read(uint8_t buf[n], static const size_t n, size_t bytes_to_read, size_t &num_bytes_read);
    fs_result_t seek(size_t offset, int seek_from_sof);
    fs_result_t size(size_t &filesize_bytes);
    fs_result_t close();
    fs_result_t unmount();
} fs_basic_if;

typedef interface fs_if {
    fs_result_t mount();
    fs_result_t open(char path[n], static const size_t n);
    fs_result_t read(uint8_t buf[n], static const size_t n, size_t bytes_to_read, size_t &num_bytes_read);
    fs_result_t seek(size_t offset, int seek_from_sof);
    fs_result_t size(size_t &filesize_bytes);
    fs_result_t close();
    fs_result_t unmount();
    fs_result_t open_mode(char path[n], static const size_t n, unsigned mode);
    fs_result_t write(const uint8_t buf[n], static const size_t n, size_t bytes_to_write, size_t &num_bytes_written);
    fs_result_t flush();
    fs_result_t tell(size_t &offset);
} fs_if;

[[distributable]]
void filesystem_basic(server interface fs_basic_if i_fs[n_fs_clients],
                      static const size_t n_fs_clients, fs_format_t fs_format,
                      client interface fs_storage_media_if i_storage_media);
[[distributable]]
void filesystem(server interface fs_if i_fs[n_fs_clients],
                static const size_t n_fs_clients, fs_format_t fs_format,
                client interface fs_storage_media_if i_storage_media);
[[distributable]]
void filesystem_block(server interface fs_if i_fs[n_fs_clients],
                      static const size_t n_fs_clients, fs_format_t fs_format,
                      client interface fs_block_media_if i_storage_media);
[[distributable]]
void filesystem_basic_block(server interface fs_basic_if i_fs[n_fs_clients],
                            static const size_t n_fs_clients, fs_format_t fs_format,
                            client interface fs_block_media_if i_storage_media);
/* Adapt an existing tile-local storage server to the bounded interface. The
 * bridge and legacy media server must be on the same tile. */
[[distributable]]
void fs_storage_media_bridge(server interface fs_block_media_if block,
                             client interface fs_storage_media_if legacy);
#endif
#endif
