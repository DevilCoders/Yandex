#!/usr/bin/python3 -u

"""The purpose of this piece of software is to convert Access Service's server.log
from its ingenious multiline format to a more parsing-friendly one (TSKV)."""

import re
import sys


SPEC_SYMBOLS_RE = re.compile('[^;]*;[^;]*;[^;]*;')
RECORD_RE = re.compile(r'(?P<timestamp>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}),\d{3} '
                       r'\[(?P<source>[^\[\]]*)\] (?P<level>[A-Z]+) (?P<data>.*)')

current = {}
spec_symbols = ''
while True:
    line = sys.stdin.readline().strip()
    if line == '':
        break
    m = SPEC_SYMBOLS_RE.match(line)
    if m:
        spec_symbols = m.group(0)
        line = line[len(spec_symbols):]
    m = RECORD_RE.match(line)
    if m:
        if current:
            print('{}timestamp={timestamp}\tsource={source}\tlevel={level}'
                  '\tdata={data}'.format(spec_symbols, **current))
        current = m.groupdict()
    else:
        if current:
            current['data'] += '\\n' + line

if current:
    print('timestamp={timestamp}\tsource={source}\tlevel={level}\tdata={data}'.format(**current))
