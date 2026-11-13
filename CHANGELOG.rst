Filesystem library change log
=============================

0.1.0
-----

  * ADDED: Functional bounded C core and XC file services, independent full-FatFs
    client handles, file creation/write/flush and explicit close/unmount.
  * ADDED: Cross-tile block-media interface and bridge for tile-local adapters.
  * FIXED: Buffer bounds, remote-reference marshalling, error propagation and
    exclusive ownership of open files.
  * CHANGED: Full FatFs comes from lib_fatfs; Petit comes from a pinned upstream
    submodule. Vendored filesystem implementations are removed.
  * CHANGED: Image creation uses current FatFs, binary streaming I/O and explicit
    overwrite protection. QSPI becomes optional and explicitly read-only.
  * ADDED: Native sanitizer and target cross-tile service tests; XCommon CMake.
  * ADDED: Basic RAM-media example with mount/open/read/close/unmount and xSCOPE
    output. Backend selection uses the optional application filesystem_conf.h.
  * ADDED: FILESYSTEM_MAX_CLIENTS configuration, defaulting to one in the public
    configuration header; full-backend file/path arrays follow the selected limit.
  * ADDED: Native image-tool C-header output; tools-only and basic example builds
    no longer require Python. Python dependencies are confined to tests.
  * CHANGED: Backend adapters and Petit wrapper/configuration are grouped in
    src/filesystems, separate from the core service and upstream sources.
  * REMOVED: Waf, legacy Make-based XCommon and native Make wrappers. Obsolete
    placeholder tests are replaced by native and cross-tile service regressions.
  * Changes to dependencies:

    - lib_xassert: 2.0.0 -> 4.3.2
    - lib_fatfs: Added pinned R0.16 integration
    - PetitFatFs: Added R0.03a.post1 submodule

0.0.1
-----

  * Initial version

  * Changes to dependencies:

    - lib_xassert: Added dependency 2.0.0
