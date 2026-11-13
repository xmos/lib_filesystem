lib_filesystem user guide
==========================

Choosing lib_fatfs or lib_filesystem
------------------------------------

``lib_fatfs`` can be used directly and this library is not required to use FAT
on xcore. Choose by where the filesystem's users are:

=============================================  =================================
Application                                    Use
=============================================  =================================
One task, same tile as the storage driver      ``lib_fatfs`` directly
Client or storage driver on another tile       ``lib_filesystem``
Several tasks opening files                    ``lib_filesystem``
Read-only, smallest footprint (Petit FatFs)    ``lib_filesystem``, Petit backend
Directories, long names, delete, format        ``lib_fatfs`` directly
=============================================  =================================

Direct use means calling ``f_*`` from one task and supplying ``diskio``
callbacks in the application. It exposes the complete FatFs API and
configuration, and reads or writes whole runs of sectors straight to or from the
caller's buffer. It is confined to one tile, because FatFs passes raw pointers,
and to one calling task, because ``lib_fatfs`` is built with
``FF_FS_REENTRANT 0`` and ``FF_FS_LOCK 0``.

``lib_filesystem`` exists for what direct use cannot do:

* a file service and a bounded media interface that work across tiles;
* one owner task serialising up to ``FILESYSTEM_MAX_CLIENTS`` clients;
* the Petit FatFs backend behind the same client interface. There is no
  standalone Petit library; this is the supported way to use it;
* the native ``fat_image_creator`` and the storage-media adapters.

``fat_image_creator`` is useful to every FAT user, not only to service users.
It is a host program built with native CMake, and its images are read equally by
direct FatFs, by Petit and by the service. An application that calls ``lib_fatfs``
directly can build and use it without linking the service; see
`Building and native image creation`_.

Cost of the service
~~~~~~~~~~~~~~~~~~~

Measured on ``examples/app_filesystem_basic`` (XS3, XTC 15.3.1,
``filesystem_basic_block``, one client), in addition to full FatFs's own
12.1 KB of code and 1.15 KB of data:

* code: about 2.9 KB, of which about 250 bytes is ``diskio`` glue a direct
  application would also need;
* static data: about 300 bytes, mostly the 256-byte ownership path;
* stack: about 1.4 KB deeper on a read (512-byte transfer and 256-byte path
  buffers in the service, a further 512-byte buffer in the media shim).

The full ``fs_if`` service is somewhat larger; it has not been measured
separately. Each further client slot reserves one ``FIL`` (552 bytes) and one
256-byte path.

The service passes at most 512 bytes to the backend per call, so FatFs never
issues a multi-sector read or write. Every sector costs one media interface
transaction and two extra copies (three when the file position is not sector
aligned). Throughput has not been benchmarked; expect direct FatFs to be faster
for large sequential transfers, particularly on SD and USB media. Each ``open``
also validates and normalises the path and checks it against other open handles.

Architecture and ownership
--------------------------

The library contains a portable C filesystem core, an XC service front end and
storage-media adapters. Full FatFs, from the separately maintained ``lib_fatfs``,
is the default backend. An optional application header selects Petit instead.
Backend selection applies to the whole application, not individual clients.

Backend adapters live under ``lib_filesystem/src/filesystems/fatfs/`` and
``lib_filesystem/src/filesystems/petit/``. The latter holds Petit's wrapper,
private configuration and disk callbacks; upstream code remains pristine in
``third_party/PetitFatFs``. Common ownership and XC service/media code stay in
``lib_filesystem/src/``.

Run **one filesystem owner task per application**. All file operations must go
through it; do not independently use direct FatFs alongside a service against
the same storage. ``fs_bind()`` installs one device, and the service claims that
core once. This does not provide cross-tile locks for several independent owners.
Users who want direct ``f_*`` calls should use ``lib_fatfs`` without the service.

``fs_if`` and ``fs_basic_if`` clients may be on other tiles. Each full-backend
client index owns one of ``FILESYSTEM_MAX_CLIENTS`` statically allocated file
handles (one by default). One file may
not be opened by another handle until it closes, even through case/separator
aliases. Petit has a single file shared by clients, returning ``FS_RES_BUSY``
until its owner closes it. Every service call is synchronous; do not put file
requests inside a timing-critical peripheral loop that cannot tolerate waiting.

The service exposes one mounted volume (drive 0). Paths are rooted on it and
contain ASCII 8.3 components. No drive prefixes, relative dot components, long
names, whitespace or trailing dots are accepted. Upper/lower ASCII case is
normalised for ownership checks. Direct FatFs has its own broader API/configuration.

The target core allocates no heap memory. Each XC transfer uses a fixed 512-byte
local buffer; paths use a fixed 256-byte local buffer. Requests may be larger
than 512 bytes and are processed in bounded chunks. Capacity and byte count are
separate parameters; oversize requests fail before accessing the buffer.

Backend configuration
---------------------

Place an optional ``filesystem_conf.h`` on the application's include path (for
example, in ``src/`` listed in ``APP_INCLUDES``). To use Petit, define::

    #define FILESYSTEM_BACKEND FILESYSTEM_BACKEND_PETIT

Omitting the header or leaving ``FILESYSTEM_BACKEND`` undefined selects
``FILESYSTEM_BACKEND_FATFS``. These constants and defaults are provided by
``filesystem_config.h``, included by ``filesystem.h``; do not include the
application configuration directly. Unknown backend values are compile errors.
XCommon CMake registers the header through ``LIB_OPTIONAL_HEADERS`` and defines
``__filesystem_conf_h_exists__`` when it is present. Native GCC/Clang builds also
discover the optional header through ``__has_include``; other native toolchains
can supply the same discovery macro when providing a header.

There is no CMake backend option. The same source list is compiled in either
configuration, with backend-specific adapters guarded by the configuration.
Old build directories containing ``FS_BACKEND`` are rejected: remove that cache
entry (``cmake -UFS_BACKEND ...``) or start with a new build directory.
XCommon CMake resolves ``lib_fatfs`` in both cases, but no full-FatFs code/state
is retained in a Petit executable. Petit's disk callbacks are privately named
to avoid its incompatible ``disk_initialize`` signature colliding with full
FatFs during link-time type checking. The host image creator always uses full
FatFs, independently of target configuration, because Petit cannot format or
create files.

Client capacity
~~~~~~~~~~~~~~~

``api/filesystem_config.h`` defaults ``FILESYSTEM_MAX_CLIENTS`` to ``1``. To
reserve more client slots, override it in the application's ``filesystem_conf.h``::

    #define FILESYSTEM_MAX_CLIENTS 4

Supported values are 1 through 8; out-of-range values are compile errors. The
public ``FS_MAX_CLIENTS`` constant, full-backend file/path arrays and service
client-count checks all use this configured limit. A service's ``n_fs_clients``
must not exceed it. Full FatFs reserves one ``FIL`` and one 256-byte ownership
path per configured slot, not an unconditional eight. Petit may have several
configured clients but still permits only one active file across them.

Client API
----------

Both interfaces support ``mount``, ``open`` (read-only), ``read``, ``seek``,
``size``, ``close`` and ``unmount``. ``fs_if`` additionally offers ``open_mode``,
``write``, ``flush`` and ``tell``. ``flush`` avoids the XC ``sync`` macro name.

``open(path, n)`` requires a NUL within ``n``, with ``n <= FS_MAX_PATH``.
``read(buffer, capacity, requested, actual)`` and ``write`` require requested
bytes not to exceed capacity. ``actual`` is set on every return, including
errors. A successful zero-byte read at EOF is normal. A partial write with a
full volume returns ``FS_RES_FULL`` and the number of bytes already written.

``open_mode`` accepts ``FS_OPEN_READ`` and/or ``FS_OPEN_WRITE`` plus either
``FS_OPEN_CREATE_NEW`` (fail if it exists) or ``FS_OPEN_CREATE_ALWAYS``. Creation
requires write access. Existing open handles are never silently replaced.
``seek(offset, from_start)`` accepts an absolute offset when from_start is true,
or a non-negative forward displacement otherwise. Seek backwards with an
absolute offset. ``size`` and ``tell`` return zero on an invalid handle.

Unmount returns ``FS_RES_BUSY`` until every file has closed. Mount on an already
mounted volume is idempotent if the requested format matches. The format enum
selects the expected FAT variant, not formatting; use ``FS_FORMAT_AUTO`` to
accept FAT12/16/32. The service does not format or enumerate directories.

Petit supports only read-only open/read/seek/size/tell/close. Write or creation
returns ``FS_RES_UNSUPPORTED``; flush of an open read-only handle is a no-op.
Petit seeks beyond EOF are rejected. Petit has no unmount call, and
``pf_mount(NULL)`` dereferences NULL; ``petit_wrapper.c`` detaches the internal
pointer in that TU instead. This workaround and configuration are outside the pristine submodule.

Storage APIs
------------

``fs_block_media_if`` is the recommended bounded interface. It supports status,
initialisation, byte reads, whole-sector writes and control operations. Reads
start at ``sector * 512 + offset`` and may cross sector boundaries. All media
must expose 512-byte logical sectors. Counts are **bytes**, not sector counts.
Readonly media report ``FS_MED_WRITE_PROTECTED`` rather than ``FS_MED_OK``.

Control commands are ``FS_CTRL_SYNC`` (no output), ``FS_GET_SECTOR_COUNT``
(uint32_t), ``FS_GET_SECTOR_SIZE`` (uint16_t) and ``FS_GET_BLOCK_SIZE`` (uint32_t,
erase blocks in logical sectors). Media must bounds-check requests against their
actual capacity and reject unsupported commands. Fixed-size control buffers are
marshalled by the bridge; no remote pointer is dereferenced by the C core.

``filesystem_block()`` and ``filesystem_basic_block()`` consume this interface.
A typical application is::

    interface fs_if files[1];
    interface fs_block_media_if media;
    par {
        on tile[0]: application_media(media);
        on tile[1]: filesystem_block(files, 1, FS_FORMAT_AUTO, media);
        on tile[0]: application(files[0]);
    }

The original ``fs_storage_media_if`` and ``filesystem()``/``filesystem_basic()``
entry points remain. Its raw-pointer operations restrict media and filesystem
tasks to the **same tile**. Adapt an old media implementation to remote use with
``fs_storage_media_bridge(block, legacy)`` on the legacy media's tile, then use
``filesystem_block()`` elsewhere. The bridge owns no filesystem instance.

The portable ``filesystem_core.h`` entry points offer the same handle operations
for a C owner task. ``fs_block_device_t`` supplies synchronous callbacks; every
target callback must carry the corresponding ``FS_MEDIA_CALLBACK`` annotation.
Callbacks operate on local buffers only and must not retain pointers. The full
backend supplies FatFs's global disk-I/O symbols, so do not link another disk-I/O
provider into that service executable. Image tools and direct FatFs apps are
separate executables, not competing providers in one program.

QSPI adapter
------------

``FS_ENABLE_QSPI=ON`` compiles ``qspi_flash_fs_media`` and links quadflash;
otherwise neither is required. The adapter exposes the quadflash **data
partition**, not factory/upgrade images. Logical sectors are 512 bytes and reads
are bounds-checked. It is read-only: raw flash requires a safe erase/rewrite and
endurance policy before writable FAT can be offered. Do not treat the previous
512-byte scratch implementation as safe for larger flash erase sectors.
Physical QSPI behaviour requires board validation. USB and SD drivers are
application supplied. The basic example provides a bounded RAM-media implementation.

Basic example
-------------

Start with ``examples/app_filesystem_basic``. It mounts an embedded read-only
FAT12 image, opens ``HELLO.TXT``, prints bounded reads through xSCOPE, closes the
file and unmounts. No USB stick, SD card or programmed QSPI data partition is
needed.

``main.xc`` connects three tasks: a read-only RAM media server on tile 0,
exactly one filesystem owner on tile 1, and a reader on tile 0. Only the owner
calls the filesystem backend. Both interfaces are bounded and cross-tile safe.
The reader exits the demonstration after a PASS marker; the library servers
normally remain running for the life of an application.

Build and run
~~~~~~~~~~~~~

Place a ``lib_fatfs`` checkout beside ``lib_filesystem`` and initialise
both repositories' submodules recursively. From the lib_filesystem root, in a
normal host shell, build the image tool using native CMake::

    cmake -S . -B build-native -DBUILD_TESTING=OFF
    cmake --build build-native --target fat_image_creator -j 8

Activate XTC Tools 15.3.1, then configure the target with XCommon CMake::

    source /Applications/XMOS_XTC_15.3.1/SetEnv.sh
    cmake -G "Unix Makefiles" -S examples/app_filesystem_basic -B build-example
    xmake -C build-example -j 8
    xrun --xscope --xscope-io-only build-example/bin/app_filesystem_basic.xe

The default board is XK-EVK-XU316-AIV (xcore.ai Vision Development Kit) with its
on-board XTAG4 xSCOPE link. Override ``APP_HW_TARGET`` for another two-tile target,
for example ``-DAPP_HW_TARGET=XCORE-200-EXPLORER`` for an XS2 build. Use the board's
correct xSCOPE wiring. On Windows/Linux activate XTC using its platform-specific
environment script instead of the macOS command above.

For a different host build directory, pass
``-DFAT_IMAGE_CREATOR=/absolute/path/to/fat_image_creator`` (``.exe`` on Windows).
Configuration uses the native image tool's ``--c-header`` output to embed a
64 KiB FAT12 image from ``examples/app_filesystem_basic/data/HELLO.TXT``.
The generated header stays in the build tree; editing the text file regenerates
it at the next build. Neither the image tool nor the example requires Python.

The final output must include::

    Hello from lib_filesystem!
    One owner, bounded reads, no storage hardware required.
    PASS: mounted, read HELLO.TXT, closed and unmounted

Simulator smoke test (also impose a host process timeout in automation)::

    xsim --max-cycles 10000000 --xscope "-offline example.xmt" \
      build-example/bin/app_filesystem_basic.xe

A cycle limit, timeout, silence or successful build alone is not a pass.

Selecting Petit
~~~~~~~~~~~~~~~

Full FatFs is the default. Uncomment this line in
``examples/app_filesystem_basic/src/filesystem_conf.h`` to select Petit, then
rebuild::

    #define FILESYSTEM_BACKEND FILESYSTEM_BACKEND_PETIT

There is no CMake backend option. The same reader and media server work with
either backend; the log identifies which is active. Petit is read-only and has
one active file across all clients. Deleting the optional header also selects
the full default. Replace ``ram_media()`` with an application-supplied block
driver for real media; preserve its bounds checks and single-owner policy.

Building and native image creation
-----------------------------------

Place lib_filesystem and a lib_fatfs checkout beside one another.
Initialise submodules recursively. For target applications add ``lib_filesystem``
to XCommon CMake dependencies and optionally provide ``filesystem_conf.h``.
XCommon resolves existing sibling checkouts without checking their revision, so
make sure an existing ``lib_fatfs`` checkout is at the revision this library's
dependency declaration names.
Only XCommon CMake target builds and native CMake tools/tests are supported;
Waf, legacy Make-based XCommon and native Make wrappers have been removed.

Build the image tool with an ordinary host compiler and native CMake, without
Python or the test dependencies::

    cmake -S . -B build-native -DBUILD_TESTING=OFF
    cmake --build build-native --target fat_image_creator -j 8

Create a superfloppy FAT image (volume begins at byte 0; no MBR)::

    build-native/fat_image_creator --sectors 32768 assets.img HELLO.TXT VIDEO.M1V

The default is 32768 512-byte sectors (16 MiB). ``--fat32`` requests FAT32; use
sufficiently large geometry. Otherwise
FatFs selects FAT12/FAT16. Files are imported to the root in binary mode using
a 4096-byte streaming buffer; no entire-file allocation is used. Duplicate root
names and invalid 8.3 names are rejected. Existing output is not overwritten
unless ``--force`` is specified explicitly. Failed creation retains partial
output and returns nonzero; do not flash a failed image. Timestamps are fixed
at 2026-01-01 for reproducibility. Geometry and allocation remain FatFs decisions.

Set ``FATFS_REPO`` when configuring native CMake if the ``lib_fatfs``
checkout is elsewhere.

For embedded RAM media, ``--c-header`` writes a C initializer instead of a binary
image, with the same geometry, file import and overwrite rules::

    build-native/fat_image_creator --c-header --sectors 128 image.h HELLO.TXT

The generated header declares ``static unsigned char image[<image size>]``. The
tool formats a temporary binary file and streams its bytes into the header; it
does not allocate an entire image in memory. Temporary binary storage is removed
automatically. Do not flash the textual header as a binary disk image.

Testing
-------

Native tests
~~~~~~~~~~~~

Python is used by test runners/generators, not by the library or basic example.
Use a virtual environment and a separate native test build::

    python3 -m venv .venv
    source .venv/bin/activate
    cmake -S . -B build-tests -DBUILD_TESTING=ON \
      -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
    cmake --build build-tests -j 8
    ctest --test-dir build-tests --output-on-failure

One native build runs both backend regressions with separate fixtures. The full
default test has no optional header and exercises the one-client default. The
multi-client full and Petit tests supply ``tests/config/fatfs/filesystem_conf.h``
and ``tests/config/petit/filesystem_conf.h`` respectively, explicitly requesting
eight client slots. Configuration tests check default/overridden client limits,
full/Petit headers, optional-header discovery and rejection of invalid values.
Symbol-table checks reject accidental linkage of the unselected backend/state.
``FS_SANITIZE`` defaults to ``ON`` for tests and ``OFF`` for tools-only builds;
an explicit cache value overrides either default.

Native tests create FAT12/16/32 images, verify imported binary files byte-for-byte,
exercise repeated open/read/seek/close, independent handles, malformed requests,
read errors, write/flush, full-volume partial writes and remount. They check
no-overwrite protection and byte-identical C-header output.

Target service tests
~~~~~~~~~~~~~~~~~~~~

The finite target fixture embeds a generated 64 KiB RAM image so it does not
depend on slow runtime debugger file reads. Media and the filesystem are on
different tiles. Full-backend clients read and write concurrently through the
single owner; Petit clients run sequentially because only one file can be open.
The fixture uses ``tests/config/fatfs/filesystem_conf.h`` by default to request
multiple client slots rather than changing the library's one-client default.
Create the embedded image and build for the connected board::

    build-tests/fat_image_creator --sectors 128 service.img \
      build-tests/fixtures-fatfs/TEST0.BIN build-tests/fixtures-fatfs/TEST1.BIN \
      build-tests/fixtures-fatfs/EMPTY.BIN
    source /Applications/XMOS_XTC_15.3.1/SetEnv.sh
    export PATH="/usr/bin:$PATH"
    cmake -G "Unix Makefiles" -S tests/test_service -B build-service \
      -DFS_TEST_IMAGE="$PWD/service.img" \
      -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
    xmake -C build-service -j 8
    xrun --xscope --xscope-io-only build-service/bin/test_service.xe

For Petit, use a separate build directory and add
``-DFS_TEST_CONFIG_DIR="$PWD/tests/config/petit"``. This adds an optional-header
include directory, not a CMake backend setting. The fixture derives its client
scheduling and writable tests from the same header as the library.

The default XN is XK-EVK-XU316-AIV, with the on-board XTAG4 xSCOPE link. Supply
APP_HW_TARGET for another board; do not assume the older explorer board's xLINK
wiring is interchangeable. Use xSCOPE logging, not JTAG --io. A passing run
prints a PASS marker and terminates normally. Merely building, silence, a host
timeout or simulator cycle exhaustion is not a pass. For simulation, bound
both cycles and host time; RAM tests do not validate actual QSPI/USB/SD timing.

The 2026-11-13 checks passed both native sanitizer suites and the full/Petit
cross-tile service tests on XK-EVK-XU316-AIV hardware. Full-backend tests include
write/flush/readback; Petit runs read clients sequentially. Legacy media and the
bounded bridge also passed on hardware. XS2 full-backend build/simulator and
XS3 simulator checks passed. QSPI was compile-checked only. Select
``FS_TEST_MEDIA=bridge`` or ``legacy`` and optionally ``FS_TEST_BASIC=ON`` to
exercise compatibility entry points. The obsolete QSPI placeholder tests and
standalone Petit test are superseded by these maintained suites. No USB-player
dependencies were changed.

Header-selected full/Petit native and hardware checks, the basic example with
both backends, and its XS2/XS3 simulator runs also pass after the CMake migration.
Native and XTC symbol-table checks verify the unselected backend is absent.

These tests cover valid generated images and explicit I/O faults; they do not
establish a malformed-media security audit or physical peripheral conformance.

Provenance
----------

Full FatFs comes from ``lib_fatfs``, which wraps the ``xmosforks/FatFs`` mirror
of the upstream release and its patches. Petit comes from the
``xmosforks/PetitFatFs`` submodule, an import of the upstream release archive
with upstream's published patch applied (``R0.03a.post1``). The ``lib_fatfs``
revision in use is recorded by the dependency declaration; the forks' own
READMEs and commit messages record the upstream sources and checksums.

Old vendored versions remain in this repository's history, not the active build.
The official Petit patch download is unavailable, so the fork applies an
Internet Archive copy that cannot be checked against an official checksum. The
patch fixes the CP857 table and a FAT32-disabled warning, neither of which this
library's Petit configuration compiles.
