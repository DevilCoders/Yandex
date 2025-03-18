#!/usr/bin/env python

import sys

def main():
    fnames = []
    for i in sys.stdin:
        fnames.append(i.strip().strip('",'))

    print '\t'.join(fnames)

if __name__ == "__main__":
    main()
