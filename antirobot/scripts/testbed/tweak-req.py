#!/usr/bin/env python

import sys
import re
from optparse import OptionParser
import Cookie
import random
import time
import urllib

def ParseArgs():
    parser = OptionParser(\
'''Usage: %prog [options] [file-with-request]
This script read a request and changes its features.
'''
)
    parser.add_option('', '--host', dest='host', action='store', type='string', help='replace Host: header with this value')
    parser.add_option('', '--ip', dest='ip', action='store', type='string', help='IP address or "random"')
    parser.add_option('', '--spravka', dest='spravka', action='store', help='Use this value as spravka cookie')
    parser.add_option('', '--fuid', dest='fuid', action='store', help='Use this value as fuid cookie')
    parser.add_option('', '--ts', dest='ts', action='store', help='Use this value for request timestamp. Current time is default value')
    parser.add_option('', '--batch', dest='batch', action='store_true', help='read urlencoded requests from stdin in form of <timestamp>\\t<request>')

    (opts, args) = parser.parse_args()

    return (opts, args);


X_START_TIME = 'X-Start-Time'
X_FFY = 'X-Forwarded-For-Y'
COOKIE = 'Cookie'
HOST = 'Host'

reFlags = re.MULTILINE | re.IGNORECASE

reHost = re.compile(r'%s: \S+' % HOST, reFlags)
reTimeStamp = re.compile(r'%s: \S+' % X_START_TIME, reFlags)
reXFFY = re.compile(r'%s: \S+' % X_FFY, reFlags)
reCookies = re.compile(r'%s: ([^\n\r]+)' % COOKIE, reFlags)


def RandomIp():
    return "%d.%d.%d.%d" % tuple([random.randint(0, 255) for _ in range(4)])


def GetCookies(req):
    cookObj = Cookie.SimpleCookie()

    cookSearch = reCookies.search(req)
    if cookSearch:
        cookObj.load(cookSearch.group(1))
        return (cookObj, True)

    return (cookObj, False)


def TweakReq(req, opts):
    req = req.strip()

    cookObj, hasCookies = GetCookies(req)

    if opts.spravka:
        cookObj['spravka'] = opts.spravka
    if opts.fuid:
        cookObj['fuid01'] = opts.fuid

    if opts.host:
        req = reHost.sub('%s: %s' % (HOST, opts.host), req)

    cookStr = '; '.join(['%s=%s' % (x[1].key, x[1].value) for x in cookObj.items()])
    if hasCookies:
        req = reCookies.sub('%s: %s' % (COOKIE, cookStr), req)
    elif cookStr:
        req = '%s\r\n%s: %s' % (req, COOKIE, cookStr)


    ts = opts.ts if opts.ts else int(time.time() * 1000000)
    req = reTimeStamp.sub('%s: %s' % (X_START_TIME, ts), req)

    ip = opts.ip
    if ip == 'random':
        ip = RandomIp()

    if ip:
        req = reXFFY.sub('%s: %s' % (X_FFY, ip), req)

    return '%s\r\n\r\n' % req


def DecodeLine(line):
    fs = line.split('\t', 1)
    if len(fs) == 2:
        return ((fs[0]), urllib.unquote_plus(fs[1].rstrip()))
    else:
        return (None, urllib.unquote_plus(fs[0].rstrip()))


def EncodeLine(ts, req):
    if ts:
        return '%s\t%s' % (ts, urllib.quote_plus(req))
    else:
        return urllib.quote_plus(req)


def main():
    opts, args = ParseArgs()

    fileHandle = open(args[0]) if len(args) else sys.stdin

    if opts.batch:
        for line in fileHandle:
            ts, req = DecodeLine(line)
            req = TweakReq(req, opts)
            print EncodeLine(ts, req)
    else:
        req = fileHandle.read()
        print TweakReq(req, opts),


if __name__ == "__main__":
    main()

