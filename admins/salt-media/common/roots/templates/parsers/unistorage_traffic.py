#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys
import re
from collections import defaultdict


def main(argv):

    result = defaultdict(dict)
    result['err'] = 0
    dict_size = {}

    local_traffic_re = re.compile(".*read ([0-9]+) bytes chunk from local group")
    remote_traffic_re = re.compile(".*read ([0-9]+) bytes chunk from remote group")
    logs = sys.stdin
    for line in logs:
        line_split = line.split('\t')
        message = line_split[9]

        local_traffic = local_traffic_re.match(message)
        remote_traffic = remote_traffic_re.match(message)

        bytes_stats = dict_size.setdefault('unistorage', {})

        if local_traffic:
            size = message.split(' ')[1]
            bytes_stats.setdefault('local', 0)
            bytes_stats['local'] += int(size)
        elif remote_traffic:
            size = message.split(' ')[1]
            bytes_stats.setdefault('remote', 0)
            bytes_stats['remote'] += int(size)
        else:
            result['err'] += 1

    # bytes_sent 
    for (k,v) in  dict_size.items():
        for status, count in v.items():
            print "{0}_{1} {2}".format(k,status,count)

if (__name__ == "__main__"):
    sys.exit(main(sys.argv))
