// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem.h"
#include <string.h>

[[distributable]]
void fs_storage_media_bridge(server interface fs_block_media_if block,
                             client interface fs_storage_media_if legacy)
{
    uint8_t local[FS_TRANSFER_BYTES];
    while (1) {
        select {
        case block.initialise() -> fs_media_status_t status: status = legacy.initialise(); break;
        case block.status() -> fs_media_status_t status: status = legacy.status(); break;
        case block.read_bytes(uint8_t buf[n], static const size_t n, uint32_t sector, uint32_t offset, uint32_t count) -> fs_result_t rc:
            rc = FS_RES_INVALID;
            if (count <= n && offset < 512) {
                rc = FS_RES_OK;
                for (unsigned done = 0; done < count && !rc;) {
                    unsigned amount = count - done < sizeof(local) ? count - done : sizeof(local);
                    unsigned position = offset + done;
                    rc = legacy.read(local, sector + position / 512, position % 512, amount);
                    if (!rc) memcpy(buf + done, local, amount);
                    done += amount;
                }
            }
            break;
        case block.write_bytes(const uint8_t buf[n], static const size_t n, uint32_t sector, uint32_t count) -> fs_result_t rc:
            rc = FS_RES_INVALID;
            if (count <= n && !(count % 512)) {
                rc = FS_RES_OK;
                for (unsigned done = 0; done < count && !rc;) {
                    unsigned amount = count - done < sizeof(local) ? count - done : sizeof(local);
                    memcpy(local, buf + done, amount);
                    rc = legacy.write(local, sector + done / 512, amount);
                    done += amount;
                }
            }
            break;
        case block.control(uint8_t command, uint8_t buf[n], static const size_t n) -> fs_result_t rc:
            rc = FS_RES_UNSUPPORTED;
            unsigned bytes = command == FS_CTRL_SYNC ? 0 : command == FS_GET_SECTOR_SIZE ? 2 : 4;
            if (command <= FS_GET_BLOCK_SIZE && n >= bytes) {
                memset(local, 0, 4);
                rc = legacy.ioctl(command, local);
                if (!rc && bytes) memcpy(buf, local, bytes);
            }
            break;
        }
    }
}
