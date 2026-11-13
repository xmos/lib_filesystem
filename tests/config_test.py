#!/usr/bin/env python3
# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""Check optional-header discovery, configuration defaults and validation."""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compiler', required=True)
parser.add_argument('--api', required=True)
args = parser.parse_args()
cases = [(None, 1, 1, None), ('', 1, 1, None)]
for setting, expected in [('FILESYSTEM_BACKEND_FATFS', 1), ('FILESYSTEM_BACKEND_PETIT', 2),
                          ('0', None), ('3', None), ('MISSPELLED_BACKEND', None)]:
    cases.append((f'#define FILESYSTEM_BACKEND {setting}\n', expected, 1,
                  'FILESYSTEM_BACKEND must be' if expected is None else None))
for limit in [1, 2, 8, 0, -1, 9, 'MISSPELLED_LIMIT']:
    valid = isinstance(limit, int) and 1 <= limit <= 8
    cases.append((f'#define FILESYSTEM_MAX_CLIENTS {limit}\n', 1, limit,
                  None if valid else 'FILESYSTEM_MAX_CLIENTS must be'))
cases.append(('#define FILESYSTEM_BACKEND FILESYSTEM_BACKEND_PETIT\n'
              '#define FILESYSTEM_MAX_CLIENTS 8\n', 2, 8, None))

with tempfile.TemporaryDirectory() as directory:
    header = Path(directory) / 'filesystem_conf.h'
    for configuration, backend, clients, error in cases:
        if configuration is None:
            header.unlink(missing_ok=True)
        else:
            header.write_text(configuration)
        source = '#include "filesystem.h"\n'
        if error is None:
            source += (f'_Static_assert(FILESYSTEM_BACKEND == {backend}, "Wrong backend");\n'
                       f'_Static_assert(FILESYSTEM_MAX_CLIENTS == {clients}, "Wrong client limit");\n'
                       '_Static_assert(FS_MAX_CLIENTS == FILESYSTEM_MAX_CLIENTS, "Wrong API limit");\n')
        # Also exercise the exact discovery macro emitted by XCommon CMake.
        for discovered in ([False] if configuration is None else [False, True]):
            command = [args.compiler, '-std=c11', '-fsyntax-only', '-x', 'c',
                       '-I', args.api, '-I', directory]
            if discovered:
                command.append('-D__filesystem_conf_h_exists__')
            result = subprocess.run(command + ['-'], input=source, text=True,
                                    capture_output=True, timeout=10)
            assert (result.returncode == 0) == (error is None), result.stderr
            if error is not None:
                assert error in result.stderr, result.stderr
print('PASS: default/optional headers, backend/client overrides and invalid value rejection')
