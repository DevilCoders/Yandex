#!/usr/bin/env python

import os
import sys

DAY_COUNT = 30
KEY_BYTE_LEN = 16


def GetContent(fileName):
    return open(fileName).read()


def PutContent(fileName, content):
    open(fileName, 'w').write(content)


def GenKey():
    return ''.join(['%02X' % ord(x) for x in os.urandom(KEY_BYTE_LEN)])


def UpdateKeys(content, requiredKeyCount):
    keys = [key for key in content.split('\n') if key]

    if len(keys) < requiredKeyCount:
        numKeysToGen = requiredKeyCount - len(keys)
    else:
        numKeysToGen = 1

    newKeys = keys[-(requiredKeyCount - numKeysToGen):] + [GenKey() for i in xrange(numKeysToGen)]
    return '\n'.join(newKeys) + '\n'


def main():
    if len(sys.argv) < 2:
        print >>sys.stderr, "Keys file must be specified"
        sys.exit(2)

    keyFile = sys.argv[1]
    content = GetContent(keyFile)
    PutContent(keyFile, UpdateKeys(content, DAY_COUNT))


if __name__ == "__main__":
    main()
