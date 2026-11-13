:orphan:

#####################################################
lib_filesystem: Filesystem and block-storage services
#####################################################

:vendor: XMOS
:version: 0.1.0
:description: Bounded filesystem services with selectable FatFs and Petit FatFs backends
:scope: General use
:category: General purpose
:keywords: filesystem, FAT, storage, flash, disk image
:devices: xcore.ai, xcore-200

*******
Summary
*******

``lib_filesystem`` provides a file service over application-supplied block storage.
The default full FatFs backend uses the separately maintained ``lib_fatfs``;
applications that do not need the service should use ``lib_fatfs`` directly.
An optional Petit FatFs backend uses a pinned ``xmosforks/PetitFatFs`` submodule.
No FatFs implementation is duplicated in this repository.

A single task on the same tile as its storage driver does not need this library.
Use it for cross-tile or multi-task file access, the Petit backend, or the FAT
image tool; see "Choosing lib_fatfs or lib_filesystem" in
``doc/rst/lib_filesystem.rst``.

**Run one filesystem owner task per application.** All filesystem operations
must be serialised through that owner. Do not run separate services or independent
direct FatFs calls against the same storage. This is an ownership rule, not a
claim that upstream FatFs cannot support multiple volumes. This service exposes
one mounted volume and a configurable client limit (one by default, up to eight).

********
Features
********

* FAT12/16/32 mounting, file reads, forward/absolute seeks, size, tell and close.
* Full FatFs: file creation, write and flush, with one independent file per client.
* Petit: one active read-only file, transferred between clients after close.
* Fixed-size buffers and file state, no target heap allocation.
* XC services with bounded cross-tile file and media transfers; portable C core.
* Existing tile-local media interface retained, with an adapter to the bounded API.
* Optional read-only QSPI data-partition adapter; no forced quadflash dependency.
* Native FAT image creator, using ``lib_fatfs``, with streaming binary file import.
* Sanitizer tests for both backends and a cross-tile RAM-media service test.
* Basic RAM-backed example in ``examples/app_filesystem_basic``: mount, open,
  read, close and unmount, with xSCOPE output and no external storage required.
* Backend configuration in optional ``filesystem_conf.h``; full FatFs by default.

************
Known issues
************

* Service paths are ASCII 8.3 components. Long names, relative ``.``/``..`` paths,
  directory enumeration and formatting through the service are not exposed.
  Use direct FatFs when these APIs are required, instead of concurrent service use.
* One file cannot be open through two service handles at once. Petit has only one
  file total. Unmount is refused until all handles have closed.
* QSPI is read-only: writable FAT on raw flash needs an erase/rewrite and endurance
  policy. The adapter does not silently rewrite erase sectors with undersized buffers.
* USB and SD physical drivers are supplied by the application, not this library.
* The Petit fork includes upstream's published patch. The official patch download
  is unavailable, so the fork applies an Internet Archive copy; see the fork's
  README. This library's configuration (CP437, FAT32 enabled) does not compile
  either patched code path. Unmount is adapted outside upstream source.
* Only XCommon CMake target builds and native CMake tools/tests are supported.
  Physical QSPI/USB/SD behaviour is not verified by RAM tests.

****************
Development repo
****************

* `lib_filesystem <https://github.com/xmos/lib_filesystem>`_

**************
Required tools
**************

* XMOS XTC Tools 15.3.1 and CMake 3.21+ for target builds.
* C11 host compiler for the native image tool and basic example's embedded image.
* Tests only: Python 3 in a virtual environment.

*********************************
Required libraries (dependencies)
*********************************

* `lib_fatfs <https://www.github.com/xmos/lib_fatfs>`_ (https://www.github.com/xmos/lib_fatfs)
* `lib_xassert <https://www.github.com/xmos/lib_xassert>`_ (https://www.github.com/xmos/lib_xassert)

******************
Third-party source
******************

The Petit backend uses Petit FatFs, a submodule at ``third_party/PetitFatFs``.

* Upstream: `Petit FatFs <https://elm-chan.org/fsw/ff/00index_p.html>`_.
* XMOS import: `xmosforks/PetitFatFs <https://github.com/xmosforks/PetitFatFs>`_.
* Upstream version: ``R0.03a.post1`` (``R0.03a`` with upstream patch 1). The source
  is not modified by XMOS.

*************************
Related application notes
*************************

* None

*******
Support
*******

This package is supported by XMOS Ltd. Issues can be raised against the software at
`www.xmos.com/support <https://www.xmos.com/support>`_ or using GitHub `issues <https://github.com/xmos/lib_filesystem/issues>`_.
