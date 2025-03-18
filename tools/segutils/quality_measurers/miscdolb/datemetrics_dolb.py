#!/usr/bin/env python
# -*- coding: utf8 -*-

import sys
from simplemaplib import *
from sengines import *

def PrintRandomQuery(r=0):
    try:
        serp = ZenSerp(numdoc=50)
        if serp:
            Print(u"\n".join([serp[0].query] + [s.url for s in serp] + [""]).encode("utf8"))
    except:
        PrintLog('pid\t%d\tFail\t%s' % (mp.current_process().pid, sys.exc_value))
                
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print sys.argv[0], " <nprocs> <nqueries>"
        exit()
    Map(int(sys.argv[1]), PrintRandomQuery, xrange(0, int(sys.argv[2])))
