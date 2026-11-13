// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"
#if FILESYSTEM_MAX_CLIENTS < 2
#error The service fixture requires at least two configured clients
#endif
#ifdef TEST_BASIC
#define FILE_INTERFACE fs_basic_if
#ifdef TEST_LEGACY
#define FILE_SERVER filesystem_basic
#else
#define FILE_SERVER filesystem_basic_block
#endif
#else
#define FILE_INTERFACE fs_if
#ifdef TEST_LEGACY
#define FILE_SERVER filesystem
#else
#define FILE_SERVER filesystem_block
#endif
#endif
#ifdef TEST_LEGACY
#define CLIENT_TILE 1
#else
#define CLIENT_TILE 0
#endif
#include "image.h"
void test_stdio_init(void);
#define CHECK(c) do { if (!(c)) { printf("FAIL: line %u\n", __LINE__); exit(1); } } while (0)

static void load_image(void)
{
    test_stdio_init();
    puts("Embedded FAT12 test image ready");
}
#if !defined(TEST_BRIDGE) && !defined(TEST_LEGACY)
[[distributable]]
static void ram_media(server interface fs_block_media_if media)
{
    load_image();
    uint8_t local[FS_TRANSFER_BYTES];
    while (1) {
        select {
        case media.initialise() -> fs_media_status_t status: status = FS_MED_OK; break;
        case media.status() -> fs_media_status_t status: status = FS_MED_OK; break;
        case media.read_bytes(uint8_t buf[n], static const size_t n, uint32_t sector, uint32_t offset, uint32_t count) -> fs_result_t rc:
            rc = FS_RES_INVALID;
            if (sector < 128 && offset < 512 && count <= n && count <= sizeof(image) - sector * 512 - offset) {
                for (unsigned done = 0; done < count;) {
                    unsigned amount = count - done < sizeof(local) ? count - done : sizeof(local);
                    memcpy(local, image + sector * 512 + offset + done, amount);
                    memcpy(buf + done, local, amount);
                    done += amount;
                }
                rc = FS_RES_OK;
            }
            break;
        case media.write_bytes(const uint8_t buf[n], static const size_t n, uint32_t sector, uint32_t count) -> fs_result_t rc:
            rc = FS_RES_INVALID;
            if (sector < 128 && count <= n && count <= sizeof(image) - sector * 512) {
                for (unsigned done = 0; done < count;) {
                    unsigned amount = count - done < sizeof(local) ? count - done : sizeof(local);
                    memcpy(local, buf + done, amount);
                    memcpy(image + sector * 512 + done, local, amount);
                    done += amount;
                }
                rc = FS_RES_OK;
            }
            break;
        case media.control(uint8_t command, uint8_t buf[n], static const size_t n) -> fs_result_t rc:
            rc = FS_RES_OK;
            if (command == FS_CTRL_SYNC) { }
            else if (command == FS_GET_SECTOR_SIZE && n >= 2) {
                uint16_t value = 512; memcpy(buf, &value, sizeof(value));
            } else if (n >= 4 && (command == FS_GET_SECTOR_COUNT || command == FS_GET_BLOCK_SIZE)) {
                uint32_t value = command == FS_GET_SECTOR_COUNT ? 128 : 1; memcpy(buf, &value, sizeof(value));
            } else rc = FS_RES_INVALID;
            break;
        }
    }
}
#endif
static void client_test(client interface FILE_INTERFACE fs, chanend done, unsigned seed, unsigned length, unsigned which)
{
    CHECK(fs.mount() == FS_RES_OK);
    printf("Client %u mounted\n", which);
    char name0[] = "TEST0.BIN", name1[] = "TEST1.BIN";
    fs_result_t rc = which ? fs.open(name1, sizeof(name1)) : fs.open(name0, sizeof(name0));
    CHECK(rc == FS_RES_OK);
    size_t size, got;
    CHECK(fs.size(size) == FS_RES_OK && size == length);
    struct { uint8_t before[16], data[701], after[16]; } buffer;
    unsigned position = 0;
    do {
        memset(&buffer, 0xa5, sizeof(buffer));
        CHECK(fs.read(buffer.data, sizeof(buffer.data), sizeof(buffer.data), got) == FS_RES_OK);
        for (unsigned i = 0; i < got; ++i) CHECK(buffer.data[i] == (uint8_t)((position + i) * 37 + seed));
        for (unsigned i = 0; i < 16; ++i) CHECK(buffer.before[i] == 0xa5 && buffer.after[i] == 0xa5);
        position += got;
    } while (got);
    CHECK(position == length);
    CHECK(fs.seek(0, 1) == FS_RES_OK);
    CHECK(fs.read(buffer.data, sizeof(buffer.data), sizeof(buffer.data) + 1, got) == FS_RES_INVALID && !got);
    CHECK(fs.close() == FS_RES_OK);
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_FATFS && !defined(TEST_BASIC)
    char write0[] = "WRITE0.BIN", write1[] = "WRITE1.BIN";
    rc = which ? fs.open_mode(write1, sizeof(write1), FS_OPEN_READ | FS_OPEN_WRITE | FS_OPEN_CREATE_ALWAYS) :
                 fs.open_mode(write0, sizeof(write0), FS_OPEN_READ | FS_OPEN_WRITE | FS_OPEN_CREATE_ALWAYS);
    CHECK(rc == FS_RES_OK);
    for (unsigned i = 0; i < sizeof(buffer.data); ++i) buffer.data[i] = (uint8_t)(i * 37 + seed);
    CHECK(fs.write(buffer.data, sizeof(buffer.data), sizeof(buffer.data), got) == FS_RES_OK && got == sizeof(buffer.data));
    CHECK(fs.flush() == FS_RES_OK);
    CHECK(fs.seek(0, 1) == FS_RES_OK);
    memset(buffer.data, 0, sizeof(buffer.data));
    CHECK(fs.read(buffer.data, sizeof(buffer.data), sizeof(buffer.data), got) == FS_RES_OK && got == sizeof(buffer.data));
    for (unsigned i = 0; i < got; ++i) CHECK(buffer.data[i] == (uint8_t)(i * 37 + seed));
    CHECK(fs.close() == FS_RES_OK);
#endif
    printf("Client %u read %u bytes and closed\n", which, position);
    outuint(done, 1); outct(done, XS1_CT_END);
}
static void finish(chanend a, chanend b)
{
    CHECK(inuint(a) == 1); chkct(a, XS1_CT_END);
    CHECK(inuint(b) == 1); chkct(b, XS1_CT_END);
    puts("PASS: cross-tile filesystem service, two clients, bounded reads and independent ownership");
    exit(0);
}

#if defined(TEST_BRIDGE) || defined(TEST_LEGACY)
[[distributable]]
static void ram_legacy(server interface fs_storage_media_if media)
{
    load_image();
    while (1) {
        select {
        case media.initialise() -> fs_media_status_t status: status = FS_MED_OK; break;
        case media.status() -> fs_media_status_t status: status = FS_MED_OK; break;
        case media.read(uint8_t *buf, uint32_t sector, uint32_t offset, uint32_t count) -> fs_result_t rc:
            rc = FS_RES_INVALID;
            if (sector < 128 && offset < 512 && count <= sizeof(image) - sector * 512 - offset) {
                memcpy(buf, image + sector * 512 + offset, count); rc = FS_RES_OK;
            }
            break;
        case media.write(const uint8_t *buf, uint32_t sector, uint32_t count) -> fs_result_t rc:
            rc = FS_RES_INVALID;
            if (sector < 128 && count <= sizeof(image) - sector * 512) {
                memcpy(image + sector * 512, buf, count); rc = FS_RES_OK;
            }
            break;
        case media.ioctl(uint8_t command, uint8_t *buf) -> fs_result_t rc:
            rc = FS_RES_OK;
            if (command == FS_CTRL_SYNC) { }
            else if (command == FS_GET_SECTOR_SIZE) { uint16_t value = 512; memcpy(buf, &value, 2); }
            else if (command == FS_GET_SECTOR_COUNT || command == FS_GET_BLOCK_SIZE) {
                uint32_t value = command == FS_GET_SECTOR_COUNT ? 128 : 1; memcpy(buf, &value, 4);
            } else rc = FS_RES_UNSUPPORTED;
            break;
        }
    }
}
#endif
int main(void)
{
    interface FILE_INTERFACE fs[2];
#ifdef TEST_LEGACY
    interface fs_storage_media_if media;
#else
    interface fs_block_media_if media;
#endif
#ifdef TEST_BRIDGE
    interface fs_storage_media_if raw;
#endif
    chan a, b;
    par {
#ifdef TEST_LEGACY
        on tile[0]: ram_legacy(media);
        on tile[0]: FILE_SERVER(fs, 2, FS_FORMAT_FAT12, media);
#else
#ifdef TEST_BRIDGE
        on tile[0]: ram_legacy(raw);
        on tile[0]: fs_storage_media_bridge(media, raw);
#else
        on tile[0]: ram_media(media);
#endif
        on tile[1]: FILE_SERVER(fs, 2, FS_FORMAT_FAT12, media);
#endif
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_FATFS
        on tile[CLIENT_TILE]: client_test(fs[0], a, 17, 8193, 0);
        on tile[CLIENT_TILE]: client_test(fs[1], b, 23, 4097, 1);
        on tile[1]: finish(a, b);
#else
        /* Petit has only one active file: run the two clients in order. */
        on tile[CLIENT_TILE]: {
            par {
                { client_test(fs[0], a, 17, 8193, 0); client_test(fs[1], b, 23, 4097, 1); }
                finish(a, b);
            }
        }
#endif
    }
    return 0;
}
