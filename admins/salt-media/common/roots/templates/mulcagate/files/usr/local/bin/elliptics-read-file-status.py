#!/usr/bin/env python

# -*- coding: UTF-8 -*-

import sys

lrc_ok = 0
lrc_err = 0
lrc_timings = []
native_ok = 0
native_err = 0
native_timings = []
err = 0

for line in sys.stdin:

    line = line.strip()
    if 'successfully read file from LRC groups' in line:
        lrc_ok += 1
        try:
            time = [x.split('=') for x in line.split(', ') if 'total_time' in x]
            lrc_timings.append(round(float(time[0][1][1:-3]) * 0.001, 3))
        except:
            err += 1
    elif 'failed to read file from LRC groups' in line:
        lrc_err += 1
    elif 'successfully read file from native groups' in line:
        native_ok += 1
        try:
            time = [x.split('=') for x in line.split(', ') if 'total_time' in x]
            native_timings.append(float(time[0][1][1:-3]) * 0.001)
        except:
            err += 1
    elif 'failed to read file from native groups' in line:
        native_err += 1

print 'lrc_ok', lrc_ok
print 'lrc_err', lrc_err
print 'native_ok', native_ok
print 'native_err', native_err
if lrc_timings:
    d = reduce(lambda x, y: dict(x.items()+[(y, x[y]+1 if y in x else 1)]), lrc_timings, dict())
    out = '@lrc_timings'
    for x, y in d.items():
        out += " %.3f@%d" % (x, y)
    print out
if native_timings:
    d = reduce(lambda x, y: dict(x.items()+[(y, x[y]+1 if y in x else 1)]), native_timings, dict())
    out = '@native_timings'
    for x, y in d.items():
        out += " %.3f@%d" % (x, y)
    print out

sys.exit(0)
