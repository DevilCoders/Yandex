#!/usr/bin/env python
import sys
import util

for reqid, params, queries, urls in util.read_sample_table(sys.stdin):
    for url, owner, source, query_index in urls:
        sys.stdout.write(url + '\n')

