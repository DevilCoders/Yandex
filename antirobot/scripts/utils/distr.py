#!/usr/bin/env python 

import sys
from optparse import OptionParser


def ParseArgs():
    parser = OptionParser('Usage: %prog [options]')
    parser.add_option('', '-f', dest='useFloat', action='store_true', help="use float numbers")
    parser.add_option('', '--min', dest='min', action='store', type='float', default=1, help="minimum in range; default 1")
    parser.add_option('', '--max', dest='max', action='store', type='float', default=100, help="maximum in range; default 100")
    parser.add_option('', '--step', dest='step', action='store', type='float', default=1, help="range step; default 1")
    parser.add_option('', '--stat', dest='stats', action='store_true', help='print some additional statistics')
    (opts, args) = parser.parse_args()
    return (opts, args);

class Distribution:
    def __init__(self, min, max, step = 1):
        self.borders = []
        self.result_map = {}
        self.min = min
        self.max = max
        self.step = step
        self.totalCount = 0

        self._InitBorders()

    def _InitBorders(self):
        self.borders = [-sys.maxint]
        start = self.min - self.step
        end = self.max + self.step
        i = start
        while i <= end:
            self.borders.append(i)
            i += self.step

        self.borders.append(sys.maxint)

        for i, _ in enumerate(self.borders):
            self.result_map[i] = 0

    def _FindBorder(self, floatVal):
        for i,b in enumerate(self.borders):
            if floatVal > b:
                continue
            return i

        return None

    def Iterate(self, iter):
        self.totalCount = 0
        for i in iter:
            value = 0
            if type(i) == '':
                value = float(i.strip())
            else:
                value = float(i)

            self.result_map[self._FindBorder(value)] += 1
            self.totalCount += 1

    # return list of tuples (time, count)
    def GetResult(self):
        keys = self.result_map.keys()
        keys.sort()

        return [(self.borders[key], self.result_map[key]) for key in keys]

    def GetQuantile(self, floatQuant):
        targetCount = self.totalCount * floatQuant

        curSumm = 0
        for i, val in enumerate(self.borders):
            curSumm += self.result_map[i]
            if curSumm >= targetCount:
                return val
        return None

def main():
    (opts, args) = ParseArgs()
    map = {}

    distr = Distribution(opts.min, opts.max, opts.step)

    distr.Iterate(sys.stdin)

    result = distr.GetResult()

    formatStr = '%f\t%d' if opts.useFloat else '%d\t%d'
    for k in result:
        print formatStr % (k[0], k[1])

    if opts.stats:
        print >>sys.stderr, "Quantile 0.25: ", distr.GetQuantile(0.25)
        print >>sys.stderr, "Quantile 0.5: ", distr.GetQuantile(0.5)
        print >>sys.stderr, "Quantile 0.75: ", distr.GetQuantile(0.75)

if __name__ == "__main__":
    main()
