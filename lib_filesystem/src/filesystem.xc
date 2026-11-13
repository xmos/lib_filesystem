// Copyright 2015-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem_core.h"
#include "xassert.h"
#include <string.h>
void fs_connect_media(client interface fs_storage_media_if media);
void fs_connect_block_media(client interface fs_block_media_if media);

/* Remote interface arrays/references cannot be passed through another XC
 * function. Keep marshalling at the case site, with fixed-size local buffers. */
#define OPEN_FILE(MODE) do { \
    rc = FS_RES_INVALID; \
    if (n && n <= FS_MAX_PATH) { \
        char local[FS_MAX_PATH]; memcpy(local, path, n); \
        if (memchr(local, 0, n)) { unsafe { rc = fs_open(i, local, MODE); } } \
    } \
} while (0)
#define READ_FILE() do { \
    uint8_t local[FS_TRANSFER_BYTES]; size_t total = 0, unused = 0; \
    rc = FS_RES_INVALID; \
    if (count <= n) { \
        unsafe { rc = fs_read(i, local, 0, &unused); } \
        while (!rc && total < count) { \
            size_t amount = count - total < sizeof(local) ? count - total : sizeof(local), done = 0; \
            unsafe { rc = fs_read(i, local, amount, &done); } \
            if (done) memcpy(buf + total, local, done); total += done; \
            if (done < amount) break; \
        } \
    } \
    actual = total; \
} while (0)
#define WRITE_FILE() do { \
    uint8_t local[FS_TRANSFER_BYTES]; size_t total = 0, unused = 0; \
    rc = FS_RES_INVALID; \
    if (count <= n) { \
        unsafe { rc = fs_write(i, local, 0, &unused); } \
        while (!rc && total < count) { \
            size_t amount = count - total < sizeof(local) ? count - total : sizeof(local), done = 0; \
            memcpy(local, buf + total, amount); \
            unsafe { rc = fs_write(i, local, amount, &done); } \
            total += done; if (done < amount) break; \
        } \
    } \
    actual = total; \
} while (0)
[[distributable]]
void filesystem(server interface fs_if i_fs[n_fs_clients], static const size_t n_fs_clients,
                fs_format_t format, client interface fs_storage_media_if media)
{
    xassert(n_fs_clients && n_fs_clients <= FS_MAX_CLIENTS);
    fs_connect_media(media);
    while (1) {
        select {
        case i_fs[int i].mount() -> fs_result_t rc: rc = fs_mount(format); break;
        case i_fs[int i].unmount() -> fs_result_t rc: rc = fs_unmount(); break;
        case i_fs[int i].open(char path[n], static const size_t n) -> fs_result_t rc: OPEN_FILE(FS_OPEN_READ); break;
        case i_fs[int i].open_mode(char path[n], static const size_t n, unsigned mode) -> fs_result_t rc: OPEN_FILE(mode); break;
        case i_fs[int i].close() -> fs_result_t rc: rc = fs_close(i); break;
        case i_fs[int i].read(uint8_t buf[n], static const size_t n, size_t count, size_t &actual) -> fs_result_t rc: READ_FILE(); break;
        case i_fs[int i].write(const uint8_t buf[n], static const size_t n, size_t count, size_t &actual) -> fs_result_t rc: WRITE_FILE(); break;
        case i_fs[int i].seek(size_t offset, int from_start) -> fs_result_t rc: rc = fs_seek(i, offset, from_start); break;
        case i_fs[int i].size(size_t &size) -> fs_result_t rc: {
            size_t local; unsafe { rc = fs_size(i, &local); } size = local; break;
        }
        case i_fs[int i].tell(size_t &offset) -> fs_result_t rc: {
            size_t local; unsafe { rc = fs_tell(i, &local); } offset = local; break;
        }
        case i_fs[int i].flush() -> fs_result_t rc: rc = fs_sync(i); break;
        }
    }
}
[[distributable]]
void filesystem_basic(server interface fs_basic_if i_fs[n_fs_clients], static const size_t n_fs_clients,
                      fs_format_t format, client interface fs_storage_media_if media)
{
    xassert(n_fs_clients && n_fs_clients <= FS_MAX_CLIENTS);
    fs_connect_media(media);
    while (1) {
        select {
        case i_fs[int i].mount() -> fs_result_t rc: rc = fs_mount(format); break;
        case i_fs[int i].unmount() -> fs_result_t rc: rc = fs_unmount(); break;
        case i_fs[int i].open(char path[n], static const size_t n) -> fs_result_t rc: OPEN_FILE(FS_OPEN_READ); break;
        case i_fs[int i].close() -> fs_result_t rc: rc = fs_close(i); break;
        case i_fs[int i].read(uint8_t buf[n], static const size_t n, size_t count, size_t &actual) -> fs_result_t rc: READ_FILE(); break;
        case i_fs[int i].seek(size_t offset, int from_start) -> fs_result_t rc: rc = fs_seek(i, offset, from_start); break;
        case i_fs[int i].size(size_t &size) -> fs_result_t rc: {
            size_t local; unsafe { rc = fs_size(i, &local); } size = local; break;
        }
        }
    }
}

/* Bounded-media entry points retain the same client operation semantics. */
[[distributable]]
void filesystem_block(server interface fs_if i_fs[n_fs_clients], static const size_t n_fs_clients,
                      fs_format_t format, client interface fs_block_media_if media)
{
    xassert(n_fs_clients && n_fs_clients <= FS_MAX_CLIENTS);
    fs_connect_block_media(media);
    while (1) {
        select {
        case i_fs[int i].mount() -> fs_result_t rc: rc = fs_mount(format); break;
        case i_fs[int i].unmount() -> fs_result_t rc: rc = fs_unmount(); break;
        case i_fs[int i].open(char path[n], static const size_t n) -> fs_result_t rc: OPEN_FILE(FS_OPEN_READ); break;
        case i_fs[int i].open_mode(char path[n], static const size_t n, unsigned mode) -> fs_result_t rc: OPEN_FILE(mode); break;
        case i_fs[int i].close() -> fs_result_t rc: rc = fs_close(i); break;
        case i_fs[int i].read(uint8_t buf[n], static const size_t n, size_t count, size_t &actual) -> fs_result_t rc: READ_FILE(); break;
        case i_fs[int i].write(const uint8_t buf[n], static const size_t n, size_t count, size_t &actual) -> fs_result_t rc: WRITE_FILE(); break;
        case i_fs[int i].seek(size_t offset, int from_start) -> fs_result_t rc: rc = fs_seek(i, offset, from_start); break;
        case i_fs[int i].size(size_t &size) -> fs_result_t rc: {
            size_t local; unsafe { rc = fs_size(i, &local); } size = local; break;
        }
        case i_fs[int i].tell(size_t &offset) -> fs_result_t rc: {
            size_t local; unsafe { rc = fs_tell(i, &local); } offset = local; break;
        }
        case i_fs[int i].flush() -> fs_result_t rc: rc = fs_sync(i); break;
        }
    }
}
[[distributable]]
void filesystem_basic_block(server interface fs_basic_if i_fs[n_fs_clients], static const size_t n_fs_clients,
                            fs_format_t format, client interface fs_block_media_if media)
{
    xassert(n_fs_clients && n_fs_clients <= FS_MAX_CLIENTS);
    fs_connect_block_media(media);
    while (1) {
        select {
        case i_fs[int i].mount() -> fs_result_t rc: rc = fs_mount(format); break;
        case i_fs[int i].unmount() -> fs_result_t rc: rc = fs_unmount(); break;
        case i_fs[int i].open(char path[n], static const size_t n) -> fs_result_t rc: OPEN_FILE(FS_OPEN_READ); break;
        case i_fs[int i].close() -> fs_result_t rc: rc = fs_close(i); break;
        case i_fs[int i].read(uint8_t buf[n], static const size_t n, size_t count, size_t &actual) -> fs_result_t rc: READ_FILE(); break;
        case i_fs[int i].seek(size_t offset, int from_start) -> fs_result_t rc: rc = fs_seek(i, offset, from_start); break;
        case i_fs[int i].size(size_t &size) -> fs_result_t rc: {
            size_t local; unsafe { rc = fs_size(i, &local); } size = local; break;
        }
        }
    }
}
