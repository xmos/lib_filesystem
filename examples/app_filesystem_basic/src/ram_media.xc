// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include "example.h"
#include "image.h"

/* A read-only FAT12 image generated from data/HELLO.TXT at configure time.
 * Replace this task with your USB/SD driver to use real storage. */
[[distributable]]
void ram_media(server interface fs_block_media_if media)
{
    uint8_t local[FS_TRANSFER_BYTES];
    while (1) {
        select {
        case media.initialise() -> fs_media_status_t status:
            status = FS_MED_WRITE_PROTECTED;
            break;
        case media.status() -> fs_media_status_t status:
            status = FS_MED_WRITE_PROTECTED;
            break;
        case media.read_bytes(uint8_t buf[n], static const size_t n,
                              uint32_t sector, uint32_t offset, uint32_t count) -> fs_result_t rc:
            rc = FS_RES_INVALID;
            if (sector < sizeof(image) / 512 && offset < 512 && count <= n &&
                count <= sizeof(image) - sector * 512 - offset) {
                for (unsigned done = 0; done < count;) {
                    unsigned amount = count - done < sizeof(local) ? count - done : sizeof(local);
                    memcpy(local, image + sector * 512 + offset + done, amount);
                    /* Copy through a local buffer: buf may belong to another tile. */
                    memcpy(buf + done, local, amount);
                    done += amount;
                }
                rc = FS_RES_OK;
            }
            break;
        case media.write_bytes(const uint8_t buf[n], static const size_t n,
                               uint32_t sector, uint32_t count) -> fs_result_t rc:
            rc = FS_RES_DENIED;
            break;
        case media.control(uint8_t command, uint8_t buf[n], static const size_t n) -> fs_result_t rc:
            rc = FS_RES_OK;
            if (command == FS_CTRL_SYNC) { }
            else if (command == FS_GET_SECTOR_SIZE && n >= sizeof(uint16_t)) {
                uint16_t value = 512;
                memcpy(buf, &value, sizeof(value));
            } else if (n >= sizeof(uint32_t) &&
                       (command == FS_GET_SECTOR_COUNT || command == FS_GET_BLOCK_SIZE)) {
                uint32_t value = command == FS_GET_SECTOR_COUNT ? sizeof(image) / 512 : 1;
                memcpy(buf, &value, sizeof(value));
            } else rc = FS_RES_INVALID;
            break;
        }
    }
}
