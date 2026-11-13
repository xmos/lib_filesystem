// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include "example.h"

static void check_result(const char operation[], fs_result_t result)
{
    if (result != FS_RES_OK) {
        printf("FAIL: %s returned %u\n", operation, (unsigned)result);
        exit(1);
    }
}

void read_hello(client interface fs_basic_if files)
{
#if FILESYSTEM_BACKEND == FILESYSTEM_BACKEND_PETIT
    puts("Backend: Petit FatFs");
#else
    puts("Backend: FatFs");
#endif
    check_result("mount", files.mount());
    char path[] = "HELLO.TXT";
    check_result("open", files.open(path, sizeof(path)));
    size_t size;
    check_result("size", files.size(size));
    printf("HELLO.TXT: %u bytes\n", (unsigned)size);

    uint8_t buffer[32];
    size_t actual, total = 0;
    do {
        check_result("read", files.read(buffer, sizeof(buffer), sizeof(buffer), actual));
        /* Use the returned length, not an assumed NUL terminator. */
        if (actual) printf("%.*s", (int)actual, buffer);
        total += actual;
    } while (actual);
    check_result("close", files.close());
    check_result("unmount", files.unmount());
    if (total != size) {
        puts("FAIL: file length mismatch");
        exit(1);
    }
    puts("PASS: mounted, read HELLO.TXT, closed and unmounted");
    /* Finish this finite demonstration, including its two server tasks. */
    exit(0);
}
