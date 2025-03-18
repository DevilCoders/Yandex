#!/usr/bin/env python

import sys
import argparse
import jsonpickle


def ParseArgs():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fields', action='store', dest='fields', default=[], help='field names to extract')

    return parser.parse_args()


def GetAttr(item, key, default):
    try:
        return item[key]
    except:
        return default


def main():
    opts = ParseArgs()

    if opts.fields:
        fields = opts.fields.split(',')
    else:
        fields = None

    for line in sys.stdin:
        item = jsonpickle.loads(line.strip())
        if fields:
            print '\t'.join((GetAttr(item, x, '-') for x in fields))
        else:
            print '\t'.join(item[x].encode('utf-8') for x in item.keys())


if __name__ == "__main__":
    main()
