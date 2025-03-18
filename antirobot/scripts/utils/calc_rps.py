#!/usr/bin/env python

import sys
from optparse import OptionParser
from collections import deque

# Reads data from stdin 
# Take first field asssuming it is timestamp value (in seconds or microseconds)
# Print rps value on stdout

def ParseArgs():
    parser = OptionParser('Usage: %prog [options]')
    parser.add_option('', '--micro', dest='micro', action='store_true', help="input time in microseconds")
    parser.add_option('', '--window', dest='window', action='store', type='int', help="number of request in calc window")
    (opts, args) = parser.parse_args()
    return (opts, args);   

def main():
    opts, args = ParseArgs()

    maxLen = opts.window if opts.window else 16
    divider = 1000000 if opts.micro else 1
    deq = deque(maxlen=maxLen)

    for i in sys.stdin:
        deq.append(float(i.split()[0]))
        if len(deq) == maxLen:
            diff = (deq[-1] - deq[0]) / divider
            rps =  maxLen / diff if diff > 0 else 0
            print rps

if __name__ == "__main__":
    main()
