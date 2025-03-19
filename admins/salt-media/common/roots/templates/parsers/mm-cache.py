#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys

out = {'mm_cache_error': 0,
       'mm_cache_skip': 0,
       'mm_cache_up': 0,
       'mm_cache_not_cache': 0}


for line in sys.stdin:

    line = line.strip()

    if '[mm.cache]' in line:
        if 'Failed to update monitor top stats' in line:
            out['mm_cache_error'] += 1
        elif 'Mb/s; cached,' in line:
            if ', skipped' in line:
                out['mm_cache_skip'] += 1
            elif 'more copies' in line and 'missing' in line:
                out['mm_cache_up'] += 1
        elif 'Mb/s; not cached,' in line:
            out['mm_cache_not_cache'] += 1

for key, val in out.iteritems():
    print key, val

sys.exit(0)
