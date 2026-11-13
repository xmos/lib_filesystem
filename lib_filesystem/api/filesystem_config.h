// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef FILESYSTEM_CONFIG_H_
#define FILESYSTEM_CONFIG_H_

#define FILESYSTEM_BACKEND_FATFS 1
#define FILESYSTEM_BACKEND_PETIT 2

/* XCommon CMake discovers application-supplied optional headers. Native
 * compilers can discover the same header on their ordinary include path. */
#ifdef __filesystem_conf_h_exists__
#include "filesystem_conf.h"
#elif defined(__has_include)
#if __has_include("filesystem_conf.h")
#include "filesystem_conf.h"
#endif
#endif

#ifndef FILESYSTEM_BACKEND
#define FILESYSTEM_BACKEND FILESYSTEM_BACKEND_FATFS
#endif

/* Reserve only as many client/file slots as the application needs. */
#ifndef FILESYSTEM_MAX_CLIENTS
#define FILESYSTEM_MAX_CLIENTS 1
#endif

#if FILESYSTEM_BACKEND != FILESYSTEM_BACKEND_FATFS && FILESYSTEM_BACKEND != FILESYSTEM_BACKEND_PETIT
#error FILESYSTEM_BACKEND must be FILESYSTEM_BACKEND_FATFS or FILESYSTEM_BACKEND_PETIT
#endif

#if FILESYSTEM_MAX_CLIENTS < 1 || FILESYSTEM_MAX_CLIENTS > 8
#error FILESYSTEM_MAX_CLIENTS must be between 1 and 8
#endif

#endif
