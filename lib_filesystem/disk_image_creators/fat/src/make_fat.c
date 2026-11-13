// Copyright 2015-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "ff.h"
#include "image_device.h"
#include "fs_internal.h"
#if defined(BYTES_PER_SECTOR) && BYTES_PER_SECTOR != 512
#error The selected FatFs integration uses 512-byte logical sectors
#endif
#ifndef SECTOR_COUNT
#define SECTOR_COUNT 32768
#endif

static int write_c_header(FILE *output, FILE *image, uint64_t bytes)
{
    uint8_t data[4096];
    if (fflush(image) || fseeko(image, 0, SEEK_SET)) return 1;
    if (fprintf(output, "/* Generated FAT image; do not edit. */\nstatic unsigned char image[%" PRIu64 "] = {\n", bytes) < 0)
        return 1;
    for (uint64_t position = 0; position < bytes;) {
        size_t count = bytes - position < sizeof(data) ? (size_t)(bytes - position) : sizeof(data);
        if (fread(data, 1, count, image) != count) return 1;
        for (size_t i = 0; i < count; ++i)
            if (fprintf(output, "%u,%c", (unsigned)data[i], (position + i) % 32 == 31 ? '\n' : ' ') < 0)
                return 1;
        position += count;
    }
    return fputs("};\n", output) == EOF;
}

int main(int argc, char **argv)
{
    uint64_t sectors = SECTOR_COUNT;
    unsigned format = FM_FAT;
    int force = 0, c_header = 0, arg = 1;
    while (arg < argc && argv[arg][0] == '-') {
        if (!strcmp(argv[arg], "--force")) force = 1;
        else if (!strcmp(argv[arg], "--c-header")) c_header = 1;
        else if (!strcmp(argv[arg], "--fat32")) format = FM_FAT32;
        else if (!strcmp(argv[arg], "--sectors") && arg + 1 < argc) {
            char *end;
            errno = 0;
            sectors = strtoull(argv[++arg], &end, 10);
            if (errno || *end || sectors < 128 || sectors > UINT32_MAX) return 2;
        } else return 2;
        ++arg;
    }
    if (arg == argc) {
        fprintf(stderr, "Usage: %s [--sectors N] [--fat32] [--force] [--c-header] output [files...]\n", argv[0]);
        return 2;
    }
    if (sectors < 128 || sectors > UINT32_MAX) return 2;
    const char *name = argv[arg++];
    fs_image_t image = {0};
    image.sectors = (uint32_t)sectors;
    FILE *output = fopen(name, force ? "w+b" : "w+bx");
    if (!output) { perror(name); return 1; }
    /* Formatting still uses binary sectors. Header mode keeps those in an
     * automatically removed temporary file and streams the initializer out. */
    image.file = c_header ? tmpfile() : output;
    if (!image.file) { perror("temporary image"); fclose(output); return 1; }
    if (fseeko(image.file, (off_t)(sectors * 512 - 1), SEEK_SET) || fputc(0, image.file) == EOF || fflush(image.file)) {
        perror("size image"); fclose(image.file); if (c_header) fclose(output); return 1;
    }
    fs_block_device_t device = fs_image_device(&image);
    if (fs_bind(&device)) { fclose(image.file); if (c_header) fclose(output); return 1; }
    uint8_t work[4096], data[4096];
    MKFS_PARM options = {format | FM_SFD, 0, 0, 0, 0};
    FATFS volume;
    FRESULT result = f_mkfs("0:", &options, work, sizeof(work));
    if (!result) result = f_mount(&volume, "0:", 1);
    while (!result && arg < argc) {
        const char *host = argv[arg++], *base = host;
        for (const char *p = host; *p; ++p) if (*p == '/' || *p == '\\') base = p + 1;
        char path[FS_MAX_PATH], canonical[FS_MAX_PATH];
        if (fs_normalize_path(base, canonical)) { result = FR_INVALID_NAME; break; }
        int length = snprintf(path, sizeof(path), "0:/%s", canonical);
        if (length < 0 || length >= (int)sizeof(path)) { result = FR_INVALID_NAME; break; }
        FILE *source = fopen(host, "rb");
        if (!source) { perror(host); result = FR_DISK_ERR; break; }
        FIL destination;
        result = f_open(&destination, path, FA_CREATE_NEW | FA_WRITE);
        int opened = !result;
        while (!result) {
            size_t count = fread(data, 1, sizeof(data), source);
            if (ferror(source)) { result = FR_DISK_ERR; break; }
            if (!count) break;
            UINT written = 0;
            result = f_write(&destination, data, count, &written);
            if (!result && written != count) result = FR_DENIED;
        }
        if (opened) {
            FRESULT closed = f_close(&destination);
            if (!result) result = closed;
        }
        if (fclose(source) && !result) result = FR_DISK_ERR;
    }
    f_mount(NULL, "0:", 0);
    if (!result && c_header && write_c_header(output, image.file, sectors * 512)) result = FR_DISK_ERR;
    if (fclose(image.file) && !result) result = FR_DISK_ERR;
    if (c_header && fclose(output) && !result) result = FR_DISK_ERR;
    if (result) { fprintf(stderr, "Image creation failed: FatFs rc=%u (partial output retained)\n", result); return 1; }
    printf("Created %s: %" PRIu64 " sectors, 512 bytes/sector%s\n", name, sectors, c_header ? " (C header)" : "");
    return 0;
}
