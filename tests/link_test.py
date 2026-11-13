#!/usr/bin/env python3
# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
"""Check that each executable retains only its selected filesystem backend."""
import argparse
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--tool', required=True, help='Native nm or XTC xobjdump')
parser.add_argument('--xcore', action='store_true', help='Use xobjdump -t instead of nm')
parser.add_argument('--fatfs', required=True)
parser.add_argument('--petit', required=True)
args = parser.parse_args()
full_symbols = ['f_mount', 'f_open', 'f_read']
petit_symbols = ['pf_mount', 'pf_open', 'pf_read']
for executable, wanted, unwanted in [(args.fatfs, full_symbols, petit_symbols),
                                    (args.petit, petit_symbols, full_symbols +
                                     ['f_write', 'f_sync', 'f_lseek', 'f_close',
                                      'Fsid', 'CurrVol', 'paths', 'files'])]:
    command = [args.tool] + (['-t'] if args.xcore else []) + [executable]
    symbols = subprocess.check_output(command, text=True, timeout=30)
    for symbol in wanted:
        assert re.search(r'\b_?' + symbol + r'$', symbols, re.MULTILINE), f'{executable}: missing {symbol}'
    for symbol in unwanted:
        assert not re.search(r'\b_?' + symbol + r'$', symbols, re.MULTILINE), f'{executable}: retained {symbol}'
print('PASS: each executable retains only the selected backend and file state')
