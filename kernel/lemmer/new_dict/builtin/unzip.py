#!/usr/bin/env python
# -*- coding: utf-8 -*-


"Uncompress gnuzip file"


import zlib
import sys


usage = "Usage: " + sys.argv[0] + " <intput file> <output file>"


def main():
    if len(sys.argv) != 3:
        print usage
        print __doc__
        sys.exit(1)

    inputFilename, outputFilename = sys.argv[1:3]
    with open(inputFilename, 'rb') as inputFile:
        with open(outputFilename, 'wb') as outputFile:
            outputFile.write(zlib.decompress(inputFile.read(), zlib.MAX_WBITS | 32))


if __name__ == '__main__':
    main()
