#!/usr/bin/env python

import sys

def ParseLine(line):
    key, freq = line.rstrip("\r\n").split("\t")
    freq = float(freq)
    return key, freq

class Accumulator(object):
    def __init__(self, key, freq):
        self.StoreData(key, freq)


    def StoreData(self, key, freq):
        self.key = key
        self.freq = freq

    def PrintTotals(self):
        print "%s\t%f" % (self.key, self.freq)

def main():
    try:
        acc = Accumulator(*ParseLine(sys.stdin.next()))
        for line in sys.stdin:
            key, freq = ParseLine(line)
            if key == acc.key:
                acc.freq += freq
            else:
                acc.PrintTotals()
                acc.StoreData(key, freq)
        acc.PrintTotals()
    except StopIteration:
        pass

    return 0

main()
