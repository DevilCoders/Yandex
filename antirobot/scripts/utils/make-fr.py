#!/usr/bin/env python

import sys

def MakeFullReq(req):
    length = len(req)

    return \
'''POST /fullreq HTTP/1.1\r
Content-Length: %d\r
\r
%s''' % (length, req)


def main():
    if len(sys.argv) < 2:
        print >>sys.stderr, "Specify input file ('-' for stdin)"
        sys.exit(1)

    f = sys.stdin if sys.argv[1] == '-' else open(sys.argv[1])

    print MakeFullReq(f.read())

if __name__ == '__main__':
    main()
