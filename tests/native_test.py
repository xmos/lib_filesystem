#!/usr/bin/env python3
# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import argparse
import hashlib
from pathlib import Path
import subprocess

def run(command, ok=True):
    p = subprocess.run([str(x) for x in command], capture_output=True, text=True, timeout=30)
    if (p.returncode == 0) != ok or 'runtime error:' in p.stderr or 'AddressSanitizer' in p.stderr:
        raise RuntimeError(p.stdout + p.stderr)
    if p.stdout: print(p.stdout, end='')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--creator', type=Path, required=True)
    parser.add_argument('--test', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    files = []
    for name, size, seed in [('TEST0.BIN', 8193, 17), ('TEST1.BIN', 4097, 23), ('EMPTY.BIN', 0, 0)]:
        path = work / name
        path.write_bytes(bytes((i * 37 + seed) & 255 for i in range(size)))
        files.append(path)
    for bits, sectors in [(12, 1024), (16, 32768), (32, 131072)]:
        image = work / f'fat{bits}.img'
        # Work products belong to this test; --force is explicit and scoped.
        command = [args.creator, '--force', '--sectors', sectors]
        if bits == 32: command.append('--fat32')
        run(command + [image, *files])
        assert image.stat().st_size == sectors * 512
        before = hashlib.sha256(image.read_bytes()).digest()
        run([args.creator, image], ok=False)
        assert hashlib.sha256(image.read_bytes()).digest() == before
        run([args.test, image, bits])
    # Header mode must embed precisely the same bytes as a binary FAT image.
    binary, header = work / 'embedded.img', work / 'embedded.h'
    run([args.creator, '--force', '--sectors', 128, binary, *files])
    run([args.creator, '--force', '--c-header', '--sectors', 128, header, *files])
    source = header.read_text()
    assert 'static unsigned char image[65536]' in source
    data = bytes(int(value) for value in source.split('{', 1)[1].split('}', 1)[0].split(',') if value.strip())
    assert data == binary.read_bytes()
    before = header.read_bytes()
    run([args.creator, '--c-header', '--sectors', 128, header], ok=False)
    assert header.read_bytes() == before
    run([args.creator, '--force', '--c-header', '--sectors', 128, header, *files])
    assert header.read_bytes() == before
    run([args.creator, '--sectors', 1, work / 'invalid.img'], ok=False)
    run([args.creator, '--force', '--sectors', 128, work / 'small.img', files[0], files[0]], ok=False)
    long_name = work / 'TOO_LONG_NAME.BIN'
    long_name.write_bytes(b'not silently truncated')
    run([args.creator, '--force', '--sectors', 128, work / 'long-name.img', long_name], ok=False)
    print('PASS: FAT12/16/32 image creation, byte-exact readback/C header and no-overwrite protection')

if __name__ == '__main__':
    main()
