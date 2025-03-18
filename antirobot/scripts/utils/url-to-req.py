#!/usr/bin/env python

import sys
import urlparse
import time
from optparse import OptionParser
import socket

def ParseArgs():
    parser = OptionParser('''Usage: %prog [options] <url string>\n
If <url string> is '-' url will be read from stdin''')

    parser.add_option('', '--host', dest='host', action='store', type='string', help='value for "Host:" header')

    (opts, args) = parser.parse_args()

    if not args:
        "You must specify url to convert from"

    return (opts, args)


def MakeServPath(parsedResult):
    res = parsedResult.path
    if parsedResult.query:
        res += '?%s' % parsedResult.query

    if parsedResult.fragment:
        res += '#%s' % parsedResult.fragment

    return res


def main():
    try:
        (opts, args) = ParseArgs()

        url = urlparse.urlparse(sys.stdin.readline().strip() if args[0] == '-' else args[0].strip())
        if not url.netloc:
            raise Exception, "Url must contain a host value"

        print "GET %s HTTP/1.1\r" % MakeServPath(url)
        print "Host: %s" % (opts.host if opts.host else url.netloc)
        print "User-Agent: Antirobot url-to-req.py\r"
        print "X-Start-Time: %d" % (time.time() * 1000000)
        print "X-Forwarded-For-Y: %s" % socket.gethostbyname(socket.gethostname())

    except Exception, ex:
        print >>sys.stderr, str(ex)
        sys.exit(2)


if __name__ == "__main__":
    main()
