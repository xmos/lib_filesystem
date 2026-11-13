// Copyright 2015-2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem.h"
#include "xassert.h"
#include <quadflash.h>
#include <string.h>

[[distributable]]
void qspi_flash_fs_media(server interface fs_storage_media_if i_media,
                        fl_QSPIPorts &ports, const fl_QuadDeviceSpec spec,
                        static const size_t sector_size)
{
    xassert(sector_size == 512 && spec.sectorLayout == SECTOR_LAYOUT_REGULAR);
    unsigned connected = 0, size = 0;
    while (1) {
        select {
        case i_media.initialise() -> fs_media_status_t status:
            if (!connected && fl_connectToDevice(ports, &spec, 1)) {
                status = FS_MED_NOT_INIT;
            } else {
                connected = 1;
                size = fl_getDataPartitionSize();
                status = FS_MED_WRITE_PROTECTED;
            }
            break;
        case i_media.status() -> fs_media_status_t status:
            status = connected ? FS_MED_WRITE_PROTECTED : FS_MED_NOT_INIT; break;
        case i_media.read(uint8_t *buf, uint32_t sector, uint32_t offset, uint32_t count) -> fs_result_t rc:
            if (!connected) rc = FS_RES_NOT_READY;
            else if (sector > size / 512 || offset >= 512 || sector * 512 > size ||
                     offset > size - sector * 512 || count > size - sector * 512 - offset) rc = FS_RES_INVALID;
            else rc = fl_readData(sector * 512 + offset, count, buf) ? FS_RES_DISK_ERR : FS_RES_OK;
            break;
        case i_media.write(const uint8_t *buf, uint32_t sector, uint32_t count) -> fs_result_t rc:
            /* FAT writes require a safe erase/rewrite layer; do not rewrite a
             * flash erase sector using a 512-byte scratch array. */
            rc = FS_RES_UNSUPPORTED; break;
        case i_media.ioctl(uint8_t command, uint8_t *buf) -> fs_result_t rc:
            rc = connected ? FS_RES_OK : FS_RES_NOT_READY;
            if (connected) {
                if (command == FS_CTRL_SYNC) { }
                else if (command == FS_GET_SECTOR_COUNT) {
                    uint32_t value = size / 512; memcpy(buf, &value, sizeof(value));
                } else if (command == FS_GET_SECTOR_SIZE) {
                    uint16_t value = 512; memcpy(buf, &value, sizeof(value));
                } else if (command == FS_GET_BLOCK_SIZE) {
                    uint32_t value = fl_getDataSectorSize(0) / 512; memcpy(buf, &value, sizeof(value));
                } else rc = FS_RES_UNSUPPORTED;
            }
            break;
        }
    }
}
