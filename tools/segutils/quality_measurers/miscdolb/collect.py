#!/usr/bin/env python
# -*- coding: utf8 -*-

import sys
from simpledolblib import *


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print sys.argv[0] + " nprocesses file_with_list_of_urls output_tar [mapping_file]"
        sys.exit()

    t = Tar(sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None)

    def do(item):
        ProcessEntry(item, t)

    if '-' == sys.argv[2]:
        Map(int(sys.argv[1]), do, sys.stdin.readlines())
    else:
        with open(sys.argv[2]) as l:
            Map(int(sys.argv[1]), do, l.readlines())
    
    t.Close()

