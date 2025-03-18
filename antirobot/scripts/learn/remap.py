#!/usr/bin/env python

import sys
import re

def Remap(fnames, remaps, factors):
    if len(fnames) != len(factors):
        raise "Count of factor names is not equal of count of factors"

    res = []
    for i in remaps:
        pos = fnames.index(i)
        res.append(factors[pos])

    return res

reClean = re.compile(r'^\"?([^\"]+)\"?')

def Clean(s):
    match = reClean.search(s)
    if match:
        return match.group(1)
    else:
        return ''


def Main():
    if len(sys.argv) < 3:
        print "Usage: %s <factor_names_file> <remap_names_file> < factors" % sys.argv[0]
        return 2

    fnames = [Clean(x.strip()) for x in open(sys.argv[1]).readlines()]
    remaps = [Clean(x.strip()) for x in open(sys.argv[2]).readlines()]

    if fnames[-1] == '':
        del fnames[-1]

    if remaps[-1] == '':
        del remaps[-1]

    for line in sys.stdin:
        factors  = line.strip().split('\t')
        print '\t'.join(Remap(fnames, remaps, factors))

if __name__ == "__main__":
    Main()
        
