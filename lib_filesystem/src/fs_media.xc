// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "filesystem_core.h"
#include "xassert.h"
#include <string.h>
static unsafe client interface fs_block_media_if medium;
static unsafe client interface fs_storage_media_if legacy;
static unsigned use_legacy;
fs_result_t fs_bind_service(void);
/* Bind outside xassert(): it expands to nothing when assertions are disabled,
 * which would leave the core unbound. A second owner is always fatal. */
static void claim_core(void)
{
    if (fs_bind_service() != FS_RES_OK) fail("Run only one filesystem owner task");
}
void fs_connect_media(client interface fs_storage_media_if media)
{
    claim_core();
    use_legacy = 1;
    unsafe { legacy = media; }
}
void fs_connect_block_media(client interface fs_block_media_if media)
{
    claim_core();
    use_legacy = 0;
    unsafe { medium = media; }
}
fs_media_status_t fs_media_initialise_xc(void) { unsafe { return use_legacy ? legacy.initialise() : medium.initialise(); } }
fs_media_status_t fs_media_status_xc(void) { unsafe { return use_legacy ? legacy.status() : medium.status(); } }
fs_result_t fs_media_read_xc(uint8_t * unsafe buffer, uint32_t sector, uint32_t offset, uint32_t count)
{
    uint8_t local[FS_TRANSFER_BYTES];
    for (unsigned done = 0; done < count;) {
        unsigned amount = count - done < sizeof(local) ? count - done : sizeof(local);
        unsigned position = offset + done;
        fs_result_t rc;
        unsafe {
            rc = use_legacy ? legacy.read(local, sector + position / 512, position % 512, amount) :
                 medium.read_bytes(local, sizeof(local), sector + position / 512, position % 512, amount);
        }
        if (rc) return rc;
        unsafe { memcpy(buffer + done, local, amount); }
        done += amount;
    }
    return FS_RES_OK;
}
fs_result_t fs_media_write_xc(const uint8_t * unsafe buffer, uint32_t sector, uint32_t count)
{
    uint8_t local[FS_TRANSFER_BYTES];
    for (unsigned done = 0; done < count;) {
        unsigned amount = count - done < sizeof(local) ? count - done : sizeof(local);
        fs_result_t rc;
        unsafe {
            memcpy(local, buffer + done, amount);
            rc = use_legacy ? legacy.write(local, sector + done / 512, amount) :
                 medium.write_bytes(local, sizeof(local), sector + done / 512, amount);
        }
        if (rc) return rc;
        done += amount;
    }
    return FS_RES_OK;
}
fs_result_t fs_media_ioctl_xc(uint8_t command, void * unsafe buffer)
{
    uint8_t local[4] = {0};
    fs_result_t rc;
    unsafe { rc = use_legacy ? legacy.ioctl(command, local) : medium.control(command, local, sizeof(local)); }
    unsafe {
        if (!rc && buffer) {
            unsigned bytes = command == FS_GET_SECTOR_SIZE ? 2 : command == FS_CTRL_SYNC ? 0 : 4;
            memcpy(buffer, local, bytes);
        }
    }
    return rc;
}
