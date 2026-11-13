#!/usr/bin/env python3
# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import sys
source, destination = map(Path, sys.argv[1:])
data = source.read_bytes()
if len(data) != 128 * 512:
    raise SystemExit('Embedded RAM media requires a 128-sector (65536-byte) FAT12 image')
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text('/* Generated test image; do not edit. */\nstatic unsigned char image[65536] = {\n' +
                       '\n'.join(','.join(str(b) for b in data[i:i+32]) + ',' for i in range(0, len(data), 32)) +
                       '\n};\n')
