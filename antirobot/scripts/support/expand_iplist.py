#!/usr/bin/env python

import sys
import httplib

RACK_HOST = 'racktables.yandex.net'

def ExpandMacro(macro):
    conn = httplib.HTTPSConnection(RACK_HOST, timeout=5)
    query = '/export/expand-fw-macro.php?macro=%s' % macro

    conn.request(\
            'GET', 
            query
            )
    resp = conn.getresponse()
    if resp.status != 200:
        print >>sys.stderr, 'Resp status: ', resp.status
        raise Exception, "Bad response"

    return '%s\n' % resp.read()


def Main():
    if len(sys.argv) < 2:
        print >>sys.stderr, '''Usage: 
    expand_iplist.py <file_with_ip_or_macroses>
'''
        sys.exit(2)

    res = ''

    lines = open(sys.argv[1]).readlines()
    for i in lines:
        i = i.strip()
        if i.startswith('@'):
            res += ExpandMacro(i[1:])
        elif not i.startswith('#'):
            res += i + '\n'

    print res

if __name__ == '__main__':
    Main()
