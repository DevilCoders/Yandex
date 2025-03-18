#!/usr/bin/env python

import re
import bisect


def ipStrToNum(ip):
    if len(ip) < 7:
        return 0

    parts = ip.split(".")
    if len(parts) != 4:
        return 0

    try:
        res = 0
        for pp in parts:
            res = res * 256 + int(pp)

        return res
    except:
        return 0


def ipNumToStr(int_str):
    val = int(int_str)
    return str(val >> 24) + '.' + str((val & 0xFF0000) >> 16) + '.' + str((val & 0xFF00) >> 8) + '.' + str(val & 0xFF)


def IsStrIPv6(ipStr):
    return ipStr.find(':') >= 0


def StripLeadZeros(s):
    z = 0
    for i in s:
        if i != '0':
            break
        z += 1

    return s[z:] if z < len(s) else '0'


def ExpandIpv6StrToList(ipStr):
    MAX_FIELDS = 8

    def RaiseBadFormat():
        raise Exception("Bad ipv6 address string")

    parts = ipStr.split('::')
    if not parts or len(parts) > 2:
        RaiseBadFormat()

    NormField = lambda x: StripLeadZeros(x).lower()

    fs = [NormField(x) for x in parts[0].split(':')]
    part1len = len(fs)
    if part1len > MAX_FIELDS or (part1len < MAX_FIELDS and len(parts) < 2):
        RaiseBadFormat()

    if part1len < MAX_FIELDS:
        part2fields = [NormField(x) for x in parts[1].split(':')]
        part2len = len(part2fields)
        if part1len + part2len < 8:
            fs.extend(['0' for i in xrange(MAX_FIELDS - part1len - part2len)])

        fs.extend(part2fields)

    if len(fs) != MAX_FIELDS:
        RaiseBadFormat()

    return fs


def FindMaxZeroSequence(list):
    maxBeg = 0
    maxLen = 0
    curBeg = None
    curLen = 0
    for i in xrange(len(list)):
        if list[i] == '0':
            curLen += 1
            if curBeg is None:
                curBeg = i
        else:
            curLen = 0
            curBeg = None

        if curLen > maxLen:
            maxBeg = curBeg
            maxLen = curLen

    return (maxBeg, maxBeg+maxLen) if maxLen > 1 else (None, None)


def NormIp6(ipStr):
    fs = ExpandIpv6StrToList(ipStr)
    (beg, end) = FindMaxZeroSequence(fs)
    if beg is not None:
        return ':'.join(fs[0:beg]) + '::' + ':'.join(fs[end:])
    else:
        return ':'.join(fs)


def NormIp4(ipStr):
    res = []
    for i in ipStr.split('.'):
        res.append(StripLeadZeros(i))
    return '.'.join(res)


def NormalizeIpStr(ipStr):
    if IsStrIPv6(ipStr):
        return NormIp6(ipStr)
    else:
        return NormIp4(ipStr)


def ExpandIpv6Str(ipStr):
    """
    Returns wide version of ipv6 string
    Assume it has been normalized before
    """

    return ':'.join(ExpandIpv6StrToList(ipStr))


def ipClassC(ipNum):
    return ipNum & 0xffffff00


def ipClassB(ipNum):
    return ipNum & 0xffff0000


def StrIpClassCv4(ipStr):
    fields = ipStr.split('.')
    fields[-1] = '0'

    return '.'.join(fields)


def StrIpClassBv4(ipStr):
    fields = ipStr.split('.')
    fields[-1] = '0'
    fields[-2] = '0'

    return '.'.join(fields)


def StrIpClassCv6(ipStr):
    fields = ExpandIpv6Str(ipStr).split(':')

    fields[-1] = '0'
    fields[-2] = '0'

    return ':'.join(fields)


def StrIpClassBv6(ipStr):
    fields = ExpandIpv6Str(ipStr).split(':')

    fields[-1] = '0'
    fields[-2] = '0'
    fields[-3] = '0'
    fields[-4] = '0'

    return ':'.join(fields)


def StrIpClassC(ipStr):
    if IsStrIPv6(ipStr):
        return NormIp6(StrIpClassCv6(ipStr))

    return StrIpClassCv4(ipStr)


def StrIpClassB(ipStr):
    if IsStrIPv6(ipStr):
        return NormIp6(StrIpClassBv6(ipStr))

    return StrIpClassBv4(ipStr)


class IpInterval:
    def __init__(self, ipBeg, ipEnd):
        self.ipBeg = ipBeg
        self.ipEnd = ipEnd

    def __str__(self):
        return '%d - %d' % (self.ipBeg, self.ipEnd)

    def __cmp__(self, other):
        if not isinstance(other, IpInterval):
            return cmp(self.ipBeg, other)

        if self.ipBeg < other.ipBeg:
            return -1

        if self.ipBeg > other.ipBeg:
            return 1

        return cmp(self.ipEnd, other.ipEnd)


class InvalidInterval(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)


class IpList:
    reIp = re.compile(r'(\d{1,4})\.(\d{1,4})\.(\d{1,4})\.(\d{1,4})')

    def __init__(self, strList, strict=False):
        self.list = []
        self.ParseStrList(strList, strict)
        self.Normalize()

    def ParseStrList(self, strList, strict):
        for i in strList:
            i = i.split('#')[0]

            ipBeg = None
            ipEnd = None
            iter = IpList.reIp.finditer(i)
            try:
                m = iter.next()
                ipBeg = int(int(m.group(1)) << 24) + int(int(m.group(2)) << 16) + int(int(m.group(3)) << 8) + int(m.group(4))
                ipEnd = ipBeg

                try:
                    m = iter.next()
                    ipEnd = int(int(m.group(1)) << 24) + int(int(m.group(2)) << 16) + int(int(m.group(3)) << 8) + int(m.group(4))
                except StopIteration:
                    pass

                if strict and ipBeg > ipEnd:
                    raise InvalidInterval("invalid interval: %s" % i)
            except InvalidInterval:
                raise
            except:
                if strict and len(i.strip()) > 0:
                    raise InvalidInterval("invalid interval: %s" % i)
                continue

            self.list.append(IpInterval(ipBeg, ipEnd))

    def Normalize(self):
        self.list.sort()

        normalized = []

        for i in self.list:
            newInterval = i
            if len(normalized) == 0:
                normalized.append(newInterval)
                continue

            lastNormalized = normalized[-1]

            if newInterval.ipEnd <= lastNormalized.ipEnd:
                continue

            # also glue together touching intervals
            if newInterval.ipBeg <= lastNormalized.ipEnd + 1:
                lastNormalized.ipEnd = newInterval.ipEnd
            else:
                normalized.append(newInterval)

        self.list = normalized

    def IpInList(self, ip):
        if isinstance(ip, type('')):
            ip = ipStrToNum(ip)

        pos = bisect.bisect_right(self.list, ip)

        if pos <= 0:
            return None

        left = self.list[pos - 1]

        if left and left.ipBeg <= ip and left.ipEnd >= ip:
            return left
        else:
            return None

    def Print(self):
        for i in self.list:
            print i


class IpIntervals:
    def __init__(self, ipsFileName):
        self.ipList = IpList(open(ipsFileName))

    def IpInIntervals(self, ip):
        return self.ipList.IpInList(ip) is not None


class Net:
    def __init__(self, ipMaskStr):
        (ip, bits) = ipMaskStr.strip().split('/')
        self.mask = ~((1 << (32 - int(bits))) - 1)
        self.net = ipStrToNum(ip) & self.mask

    def MatchIpStr(self, ip):
        ipNum = ipStrToNum(ip)

        if not ipNum:
            return False

        return self.MatchIp(ipNum)

    def MatchIp(self, ipNum):
        return ipNum & self.mask == self.net


def IpMatchMask(ipMaskNum, ipNum):
    return (ipMaskNum & ipNum) == ipMaskNum
