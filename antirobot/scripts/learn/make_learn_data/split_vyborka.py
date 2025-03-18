#!/usr/bin/env python

from random import random
from sys import argv
from os.path import basename

DEFAULT_THRESHOLD = 0.5

def split_file(input, output1, output2, threshold):
    for line in input:
        (output1 if random() < threshold else output2).write(line)


def Execute(splitThreshold, featuresHacked, featuresTweakedLearn, featuresTweakedTest):
    threshold = splitThreshold or DEFAULT_THRESHOLD

    with file(featuresHacked) as fi, \
         file(featuresTweakedLearn, 'w') as fo1, \
         file(featuresTweakedTest, 'w') as fo2:

        split_file(fi, fo1, fo2, threshold)


def main():
    if len(argv) < 4:
        print 'Usage: %s <input> <output1> <output2> [threshold=0.5]' % (basename(argv[0]),)
        return
    threshold = float(argv[4]) if len(argv) > 4 else 0.5
    with file(argv[1]) as fi, file(argv[2], 'w') as fo1, file(argv[3], 'w') as fo2:
        split_file(fi, fo1, fo2, threshold)

if __name__ == '__main__':
    main()

