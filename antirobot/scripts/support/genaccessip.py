#!/usr/bin/env python

TURBO_IPS = 'trbosrvnets.txt'
SPECIAL_IPS = 'special_ips.txt'
YANDEX_IPS = 'yandex_ips.txt'
WHITELIST_IPS = 'whitelist_ips.txt'
PRIVILEGED_IPS = 'privileged_ips'

import sys
import os
import httplib
import socket
import re
import cStringIO
import urllib2
import time
import json
from optparse import OptionParser

RACK_HOST = 'racktables.yandex.net'
NETWORK_TIMEOUT = 10
NUM_ATTEMPTS = 20
REQ_DELAY = 3

def ParseArgs():
    parser = OptionParser('Usage: %prog [options]')
    parser.add_option('--no-check-ipv4', dest='noCheckIpv4', action='store_true')
    parser.add_option('--src-dir', dest='sourceDir', action='store', type='string', help='Directory where source file placed', default='.')
    parser.add_option('--out-dir', dest='outDir', action='store', type='string', help='Out directory', default='.')
    parser.add_option('--src-file', dest='sourceFile', action='store', type='string', help='Input filename')
    parser.add_option('--out-file', dest='outFile', action='store', type='string', help='Output filename')

    (opts, args) = parser.parse_args()

    return opts


def ExpandMacro(macro):
    print >>sys.stderr, "Expanding %s..." % macro,

    url = 'https://%(host)s/export/expand-fw-macro.php?macro=%(macro)s' % { 'host': RACK_HOST, 'macro': macro }

    data = None
    for _ in xrange(NUM_ATTEMPTS):
        try:
            data = urllib2.urlopen(url, timeout=NETWORK_TIMEOUT)
            break
        except urllib2.HTTPError as err:
            if err.code in (429, 502):
                time.sleep(REQ_DELAY)
                continue
            else:
                raise err

    res = '\n'.join(data.readlines())
    print >>sys.stderr, "done"

    return res


regIp = r'\d{1,4}\.\d{1,4}\.\d{1,4}\.\d{1,4}'
_reIp4 = re.compile(r'^%(ip)s(?:/\d+|\s*-\s*%(ip)s)?$' % {'ip': regIp}) # ip, ip-net or ip-range
def IsIp4(text):
    return _reIp4.match(text) != None


_reHost = re.compile(r'^(?:[\w_\-]+\.)+[a-zA-Z]+$')
def IsHost(text):
    return _reHost.match(text) != None


_reMacro = re.compile(r'^_[a-zA-Z]\w+_$')
def IsMacro(text):
    return _reMacro.match(text) != None


_reUrl = re.compile(r'^(?:ftp|http|https)://\S+$')
def IsUrl(txt):
    return _reUrl.match(txt)


def IsValidIp6Addr(ipStr):
    try:
        addrParts = ipStr.strip().split('/', 1)
        if len(addrParts) > 1:
            net = int(addrParts[1])
            if net < 0 or net >= 128:
                return False

        subParts = addrParts[0].split('::')
        if len(subParts) > 2:
            return False

        frags = []
        for p in subParts:
            if p:
                frags.extend(p.split(':'))

        if len(frags) > 8:
            return False

        for f in frags:
            try:
                val = int(f, 16)
                if val > 0xffff:
                    return False

            except:
                return False

        return True
    except:
        return False


def GetHostIp(hostname):
    print >>sys.stderr, "Getting IP for host %s..." % hostname,

    def Flatten(addrInfo):
        addrSet = set([x[4][0] for x in addrInfo])
        return '\n'.join(addrSet)

    for i in xrange(3):
        try:
            res = Flatten(socket.getaddrinfo(hostname, None))
            print >>sys.stderr, "done"
            return res
        except socket.gaierror:
            # the host does not have any IP address
            print >>sys.stderr, "%s does not have any IP address" % hostname
            return None


def IsString(string):
    try:
        str(string)
        return True
    except:
        return False


def ParseJsonData(d):
    """
        Get all ips from json
    """
    ipList = ""
    for value in d:
        if isinstance(d, dict):
            value = d[value]
        new_ips = ""
        if isinstance(value, dict):
            new_ips = '\n' + ParseJsonData(value)
        elif isinstance(value, list):
            new_ips = '\n' + ParseJsonData(value)
        elif IsString(value) and IsIp4(str(value)):
            new_ips = '\n' + value
        elif IsString(value) and IsValidIp6Addr(str(value)):
            new_ips = '\n' + value
        ipList += new_ips
    return ipList


def GetFromJson(json_data):
    ips = ParseJsonData(json_data)
    res = '\n'.join([x for x in ips.split('\n') if len(x) > 0])
    return res


def GetFromUrl(url):
    print >>sys.stderr, "Fetching from %s..." % url,
    data = urllib2.urlopen(url, timeout=NETWORK_TIMEOUT)
    headers = dict(data.info())
    if "content-type" in headers and "application/json" in headers["content-type"]:
        json_data = json.loads(data.read())
        res = GetFromJson(json_data)
    else:    
        lines = data.readlines()
        res = '\n'.join([x.strip().split(None, 1)[0] for x in lines])

    print >>sys.stderr, "done"
    return res


def GetLineContent(line):
    fs = line.strip().split('#', 1)
    if not fs:
        return ('', '')

    if len(fs) == 1:
        return (fs[0].strip(), '')

    return (fs[0].strip(), fs[1].strip())


def TranslateOnce(text):
    lines = [x for x in text.split('\n') if x]

    needTranslate = False
    res = cStringIO.StringIO()

    for line in lines:
        i, comment = GetLineContent(line)

        if comment:
            print >>res, '#', comment

        if not i:
            continue

        if IsIp4(i):
            print >>res, i
        elif IsHost(i):
            ipStr = GetHostIp(i)
            if ipStr:
                print >>res, '# %s' % i
                print >>res, ipStr
            else:
                print >>res, '# %s [failed]' % i

        elif IsMacro(i):
            print >>res, '### BEGIN %s' % i
            print >>res, ExpandMacro(i)
            print >>res, '### END %s' % i
            needTranslate = True

        elif IsUrl(i):
            print >>res, '### BEGIN %s' % i
            print >>res, GetFromUrl(i)
            print >>res, '### END %s' % i
            needTranslate = True

        elif IsValidIp6Addr(i):
            print >>res, i

        else:
            raise Exception("Unknown item type: %s" % i)

    return res.getvalue(), needTranslate


def Translate(text):
    while True:
        text, needTranslate = TranslateOnce(text)
        if not needTranslate:
            return text


def ReadSource(fileName):
    return open(fileName).read()


def EnsureIpV4CanBeResolved():
    # Here we acquire host name which definitely has IPv4-address
    # And then we check that we can receive IPv4-address back by DNS query
    ip = '8.8.8.8'
    hostname = socket.gethostbyaddr(ip)[0]
    resolvedIps = [info[4][0] for info in socket.getaddrinfo(hostname, None)]
    if ip not in resolvedIps:
        raise Exception("It seems that IPv4 address cannot be resolved.")


def Main():
    opts = ParseArgs()

    if not opts.noCheckIpv4:
        # We need to be able to resolve IPv4 addresses
        EnsureIpV4CanBeResolved()

    open(os.path.join(opts.outDir, opts.outFile), 'w').write(Translate(ReadSource(os.path.join(opts.sourceDir, opts.sourceFile))))


if __name__ == '__main__':
    Main()
