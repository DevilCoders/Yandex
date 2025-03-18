#!/usr/bin/env python

import os
import sys
import re
import datetime
import Cookie
from collections import namedtuple

import arcadia

from dict_ci import CaselessDict
import susp_request

from antirobot.scripts.learn.make_learn_data import data_types
from antirobot.scripts.utils.ip_utils import ipStrToNum, ipNumToStr, StrIpClassC, StrIpClassB


def ReqidToDateTime(reqidStr):
    return datetime.datetime.fromtimestamp(int(reqidStr[0:10]))


RedirStat = namedtuple('RedirStat', 'redirects removals')

class RndReq(data_types.RndReqData):
    def __init__(self, rndReqData=None):
        if rndReqData:
            self.__dict__.update(rndReqData.__dict__)

        self.timestamp = ReqidToDateTime(self.Raw.reqid)
        self.ReqHasQuotes = self.Raw.request.find('%22') >= 0
        self.suspicious = susp_request.IsSuspicious(self)

    @staticmethod
    def FromString(s):
        rndReq = data_types.RndReqData.FromString(s)
        return RndReq(rndReq)

    def __str__(self):
        return '\t'.join((self.Raw.reqid, self.Raw.ip))

    def DateKey(self):
        return IpDateKey(self.Raw.ip, self.timestamp)

    def DateKeyClassC(self):
        return IpDateKey(StrIpClassC(self.Raw.ip), self.timestamp)

    def DateKeyClassB(self):
        return IpDateKey(StrIpClassB(self.Raw.ip), self.timestamp)

    def WeekKey(self):
        return IpWeekKey(self.Raw.ip, self.timestamp)

    def WeekKeyClassC(self):
        return IpWeekKey(StrIpClassC(self.Raw.ip), self.timestamp)

    def WeekKeyClassB(self):
        return IpWeekKey(StrIpClassB(self.Raw.ip), self.timestamp)

class RndRequestData:
    def __init__(self):
        self.list = {} # map: reqid => RndReq object
        self.it = None
        self.dateStart = None
        self.dateEnd = None

    def __iter__(self):
        self.it = iter(self.list)
        return self

    def __len__(self):
        return len(self.list)

    def next(self):
        return self.list[(next(self.it))]

    def Add(self, rndReqObj):
        self.list[rndReqObj.Raw.reqid] = rndReqObj

    def GetIpSet(self):
        res = set()
        for i in self.list.itervalues():
            res.add(i.Raw.ip)
            res.add(StrIpClassC(i.Raw.ip))
            res.add(StrIpClassB(i.Raw.ip))

        return res

    def GetByReqid(self, reqid):
        return self.list.get(reqid, None)

    # load records from rnd_reqdata
    @staticmethod
    def Load(iterable):
        dateStart = None
        dateEnd = None
        res = RndRequestData()

        for line in iterable:
            obj = RndReq.FromString(line)
            if obj.Raw.ip == 0:
                continue;

            res.Add(obj)

            date = obj.timestamp.date();

            if not dateStart or date  < dateStart:
                dateStart = date

            if not dateEnd or date > dateEnd:
                dateEnd = date

        res.dateStart = dateStart
        res.dateEnd = dateEnd
        print >>sys.stderr, 'Start date:', res.dateStart, ' End date:', res.dateEnd
        return res

def IpDateKey(ip, dat):
    "Return a key based on ip and date of a request"
    if type(ip) == type(12345):
        ip = ipNumToStr(ip)
    return ip + '|' + str(dat.strftime("%Y%m%d"))

def IpWeekKey(ip, dat):
    "Return a key based on ip and week number of a request date"
    if type(ip) == type(12345):
        ip = ipNumToStr(ip)

    return '%s|%s' % (ip, str(dat.isocalendar()[1]))

def IpDateFromKey(key):
    fs = key.split('|')
    return (
            ipStrToNum(fs[0]),
            datetime.datetime.strptime(fs[1], "%Y%m%d")
            )

def IpWeekFromKey(key):
    fs = key.split('|')
    return (
            ipStrToNum(fs[0]),
            int(fs[1])
            )

class RndReqFull(RndReq):
    def __init__(self, rndReq):
        super(RndReqFull, self).__init__(rndReq)
        self.__dict__.update(rndReq.__dict__)
        self.headers = CaselessDict()
        self.cookies = Cookie.BaseCookie()
        self.suspicious = susp_request.IsSuspicious(self)
        self.isAjax = False

        self.Parse()

    def Parse(self):
        headers = self.Raw.request.split('\\r\\n')[1:]
        self.timestamp = ReqidToDateTime(self.Raw.reqid)
        (self.method,
        self.request) = self.Raw.request.split()[0:2]

        for i in headers:
            pos = i.find(': ')
            if pos >= 0:
                key = i[0:pos]
                value = i[pos + 2:]
                self.headers[key] = value

        reqStr = self.Raw.request
        if reqStr.find("ajax=1") >= 0 or reqStr.find("callback=jQuery") >= 0 or reqStr.find("format=json") >= 0:
            self.isAjax = True

        self.ParseCookies()

    def ParseCookies(self):
        cookies = self.headers.get('Cookie')
        if cookies:
            try:
                self.cookies.load(cookies)
            except:
                self.cookies = Cookie.BaseCookie()

    def DateKey(self):
        return IpDateKey(self.Raw.ip, self.timestamp)

def GetRndReqFullIter(rndRequestDataFile):
    for i in open(rndRequestDataFile):
        rndReq = RndReq.FromString(i.strip())
        yield RndReqFull(rndReq)

# input: rnd_reqdata from collect features sandbox task
if __name__ == "__main__":
    list = RndRequestData.Load(sys.stdin)
#    list = LoadIpSet(sys.stdin)
#    for i in list:
#        print i
    print "Total: %s, dateStart = %s, dateEnd = %s" % (len(list), list.dateStart, list.dateEnd)
