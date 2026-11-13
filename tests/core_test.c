// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "image_device.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static unsigned char pattern(unsigned position, unsigned seed) { return (unsigned char)(position * 37 + seed); }
static int check_file(unsigned handle, const char *path, unsigned length, unsigned seed)
{
    CHECK(fs_open(handle, path, FS_OPEN_READ) == FS_RES_OK);
    size_t size = 0, position = 0;
    CHECK(fs_size(handle, &size) == FS_RES_OK && size == length);
    CHECK(fs_unmount() == FS_RES_BUSY);
    struct { unsigned char before[16], data[701], after[16]; } buffer;
    for (;;) {
        memset(&buffer, 0xa5, sizeof(buffer));
        size_t got;
        CHECK(fs_read(handle, buffer.data, sizeof(buffer.data), &got) == FS_RES_OK);
        for (unsigned i = 0; i < got; ++i) CHECK(buffer.data[i] == pattern(position + i, seed));
        for (unsigned i = 0; i < 16; ++i) CHECK(buffer.before[i] == 0xa5 && buffer.after[i] == 0xa5);
        position += got;
        if (!got) break;
    }
    CHECK(position == length);
    CHECK(fs_tell(handle, &position) == FS_RES_OK && position == length);
    CHECK(fs_seek(handle, 0, 1) == FS_RES_OK);
    if (length) {
        size_t got;
        CHECK(fs_seek(handle, 7, 0) == FS_RES_OK);
        CHECK(fs_read(handle, buffer.data, 1, &got) == FS_RES_OK && got == 1 && buffer.data[0] == pattern(7, seed));
    }
    CHECK(fs_close(handle) == FS_RES_OK);
    CHECK(fs_close(handle) == FS_RES_NOT_OPENED);
    return 0;
}
int main(int argc, char **argv)
{
    CHECK(argc == 3);
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_PETIT
    CHECK(!strcmp(fs_backend_name(), "Petit FatFs"));
#else
    CHECK(!strcmp(fs_backend_name(), "FatFs"));
#endif
    fs_image_t image = {0};
    image.file = fopen(argv[1], "r+b");
    CHECK(image.file);
    CHECK(!fseeko(image.file, 0, SEEK_END));
    image.sectors = (uint32_t)(ftello(image.file) / 512);
    fs_block_device_t device = fs_image_device(&image);
    CHECK(fs_bind(NULL) == FS_RES_INVALID);
    CHECK(fs_bind(&device) == FS_RES_OK);
    CHECK(fs_open(0, "TEST0.BIN", FS_OPEN_READ) == FS_RES_NOT_READY);
    CHECK(fs_mount((fs_format_t)-1) == FS_RES_INVALID);
    image.fail_read = 1;
    CHECK(fs_mount(FS_FORMAT_AUTO) == FS_RES_DISK_ERR);
    image.fail_read = 0;
    fs_format_t format = atoi(argv[2]) == 12 ? FS_FORMAT_FAT12 : atoi(argv[2]) == 16 ? FS_FORMAT_FAT16 : FS_FORMAT_FAT32;
    CHECK(fs_mount(format) == FS_RES_OK);
    CHECK(fs_open(FS_MAX_CLIENTS, "TEST0.BIN", FS_OPEN_READ) == FS_RES_INVALID);
    CHECK(fs_open(0, "1:TEST0.BIN", FS_OPEN_READ) == FS_RES_INVALID);
    CHECK(fs_open(0, "MISSING.BIN", FS_OPEN_READ) == FS_RES_NO_FILE);
    for (unsigned repeat = 0; repeat < 30; ++repeat) {
        CHECK(!check_file(0, "TEST0.BIN", 8193, 17));
        CHECK(!check_file(FS_MAX_CLIENTS - 1, "TEST1.BIN", 4097, 23));
        CHECK(!check_file(FS_MAX_CLIENTS - 1, "EMPTY.BIN", 0, 0));
    }
    CHECK(fs_open(0, "TEST0.BIN", FS_OPEN_READ) == FS_RES_OK);
    CHECK(fs_open(0, "TEST1.BIN", FS_OPEN_READ) == FS_RES_BUSY);
#if FILESYSTEM_MAX_CLIENTS > 1
    CHECK(fs_open(1, "/test0.bin", FS_OPEN_READ) == FS_RES_BUSY);
#endif
    size_t got;
    unsigned char buffer[1000];
    CHECK(fs_read(0, NULL, 1, &got) == FS_RES_INVALID && !got);
    CHECK(fs_read(0, NULL, 0, &got) == FS_RES_OK && !got);
    CHECK(fs_size(0, NULL) == FS_RES_INVALID);
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_PETIT
#if FILESYSTEM_MAX_CLIENTS > 1
    CHECK(fs_open(1, "TEST1.BIN", FS_OPEN_READ) == FS_RES_BUSY);
#endif
    CHECK(fs_write(0, buffer, 1, &got) == FS_RES_UNSUPPORTED && !got);
#else
#if FILESYSTEM_MAX_CLIENTS > 1
    CHECK(fs_open(1, "TEST1.BIN", FS_OPEN_READ) == FS_RES_OK);
    CHECK(fs_read(0, buffer, 1, &got) == FS_RES_OK && buffer[0] == pattern(0, 17));
    CHECK(fs_read(1, buffer, 1, &got) == FS_RES_OK && buffer[0] == pattern(0, 23));
    CHECK(fs_close(1) == FS_RES_OK);
#endif
    CHECK(fs_write(0, buffer, 1, &got) == FS_RES_DENIED && !got);
#endif
    CHECK(fs_close(0) == FS_RES_OK);
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_FATFS
    /* A close that fails because the media went away must still release the
     * handle, or the client and unmount stay busy until reboot. */
    CHECK(fs_open(0, "TEST0.BIN", FS_OPEN_READ) == FS_RES_OK);
    FILE *present = image.file;
    image.file = NULL;
    CHECK(fs_close(0) == FS_RES_NOT_OPENED);
    image.file = present;
    CHECK(fs_close(0) == FS_RES_NOT_OPENED);
    CHECK(!check_file(0, "TEST0.BIN", 8193, 17));
    unsigned write_handle = FS_MAX_CLIENTS - 1;
    CHECK(fs_open(write_handle, "WRITE.BIN", FS_OPEN_READ | FS_OPEN_WRITE | FS_OPEN_CREATE_ALWAYS) == FS_RES_OK);
    for (unsigned i = 0; i < sizeof(buffer); ++i) buffer[i] = pattern(i, 31);
    CHECK(fs_write(write_handle, buffer, sizeof(buffer), &got) == FS_RES_OK && got == sizeof(buffer));
    CHECK(fs_sync(write_handle) == FS_RES_OK);
    CHECK(fs_seek(write_handle, 0, 1) == FS_RES_OK);
    memset(buffer, 0, sizeof(buffer));
    CHECK(fs_read(write_handle, buffer, sizeof(buffer), &got) == FS_RES_OK && got == sizeof(buffer));
    for (unsigned i = 0; i < sizeof(buffer); ++i) CHECK(buffer[i] == pattern(i, 31));
    CHECK(fs_close(write_handle) == FS_RES_OK);
    CHECK(fs_open(write_handle, "WRITE.BIN", FS_OPEN_WRITE | FS_OPEN_CREATE_NEW) == FS_RES_EXISTS);
    if (image.sectors <= 1024) {
        CHECK(fs_open(write_handle, "FILL.BIN", FS_OPEN_WRITE | FS_OPEN_CREATE_NEW) == FS_RES_OK);
        fs_result_t rc = FS_RES_OK;
        unsigned calls = 0;
        while (rc == FS_RES_OK && calls++ < image.sectors) {
            rc = fs_write(write_handle, buffer, sizeof(buffer), &got);
            CHECK(got <= sizeof(buffer));
        }
        CHECK(rc == FS_RES_FULL);
        CHECK(fs_close(write_handle) == FS_RES_OK);
    }
#endif
    CHECK(fs_unmount() == FS_RES_OK);
    CHECK(fs_mount(format == FS_FORMAT_FAT12 ? FS_FORMAT_FAT16 : FS_FORMAT_FAT12) == FS_RES_NO_FILESYSTEM);
    image.readonly = 1;
    CHECK(fs_mount(format) == FS_RES_OK);
    CHECK(!check_file(0, "TEST0.BIN", 8193, 17));
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_FATFS
    CHECK(fs_open(0, "NEW.BIN", FS_OPEN_WRITE | FS_OPEN_CREATE_NEW) == FS_RES_DENIED);
#endif
    CHECK(fs_unmount() == FS_RES_OK);
    CHECK(!fclose(image.file));
    printf("PASS: %s FAT%s (%u client slots) read/seek/ownership, bounds, errors and reopen\n",
           fs_backend_name(), argv[2], (unsigned)FS_MAX_CLIENTS);
    return 0;
}
