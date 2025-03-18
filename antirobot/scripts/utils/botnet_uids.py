#!/usr/bin/env python

import sys
import os

THIS_DIR = os.path.dirname( os.path.realpath(__file__))
sys.path.append('.')
sys.path.append(THIS_DIR + '/../../../yweb/scripts/datascripts/common/')

from optparse import OptionParser
from mapreducelib import MapReduce, Record, MapReduceClient, MapReduceContext

import datetime
import time
import ip_utils
import filter_yandex

from collections import defaultdict

DEBUG = False

DEF_UNIQ_IP_THREASHOLD = 50
DEF_REQ_THREASHOLD = 1000

def ParseArgs():
    parser = OptionParser('Usage: %prog [options]')
    parser.add_option('', '--server', dest='server', action='store', type='string', help="mapreduce server")
    parser.add_option('', '--date', dest='date', action='store', type='string', help="date (in form of (YYYYMMDD) (default yesterday)")
    parser.add_option('', '--ip-min', dest='ipMin', action='store', type='int', help="uniq ip minimum")
    parser.add_option('', '--req-min', dest='reqMin', action='store', type='int', help="total request minimum")
    parser.add_option('', '--search-only', dest='searchOnly', action='store_true', help="take in account only search requests")
    (opts, args) = parser.parse_args()
    return (opts, args);

def DateFromStr(str):
    st = time.strptime(str, "%Y%m%d")
    return datetime.date(st.tm_year, st.tm_mon, st.tm_mday)

def DateToStr(dateObj):
    return dateObj.strftime('%Y%m%d')

class Map:
    SEARCH_REQ_TYPES = set([
            'web-ys',
            'img-ys',
            'web-nolr_ys',
            'web-ms',
            'web-school',
            'web-large',
            'web-family',
            'web-sitesearch']) 
    
    def __init__(self, searchOnly):
        self.searchOnly = searchOnly

    def __call__(self, rec):
        fields = rec.value.split('\t')
        if len(fields) < 9:
            return

        reqType = fields[4]
        if self.searchOnly and reqType not in self.SEARCH_REQ_TYPES:
            return

        ipStr = fields[8]

        try:
            ipNum = ip_utils.ipStrToNum(ipStr)
        except:
            return

        if filter_yandex.is_yandex(ipNum):
            return

        yield Record(fields[7], '', '\t'.join((rec.key, fields[8], reqType)))

class Reduce:
    def __init__(self, ipMin, reqMin):
        self.ipMin = ipMin
        self.reqMin = reqMin

    def __call__(self, key, reqs):
        ips = set()
        host_ip = defaultdict(lambda: 0)
        total = 0
        for r in reqs:
            total += 1
            (host, ip, reqType) = r.value.split()
            host_ip['%s\t%s' % (host, ip)] += 1
            ips.add(ip)

        if total >= self.reqMin and len(ips) >= self.ipMin:
            for k, v in host_ip.iteritems():
                yield Record(key, '', '\t'.join((k, str(v))))

def FastPrintTable(tableName):
    mr = MapReduceClient(usingSubkey = False, lenvalMode = False)
    ctx = MapReduceContext()
    shellCommand = mr.getShellBase(ctx) 
    shellCommand += ["-read", tableName]
    ctx.runOS(shellCommand)

def Main():

    (opts, args) = ParseArgs()
    server = opts.server if opts.server else os.environ.get('DEF_MR_SERVER', 'betula00:8013')
    date = DateFromStr(opts.date) if opts.date else datetime.date.today() - datetime.timedelta(1)
    ipMin = opts.ipMin if opts.ipMin else DEF_UNIQ_IP_THREASHOLD
    reqMin = opts.reqMin if opts.reqMin else DEF_REQ_THREASHOLD
    searchOnly = opts.searchOnly

    print >>sys.stderr, "ip min: %s" % ipMin
    print >>sys.stderr, "req min: %s" % reqMin

    MapReduce.useDefaults(verbose = True, workDir = ".", server = server, testMode = DEBUG)

    resultTable = 'tmp/antirobot_botnet_%s' % DateToStr(date)
    dstMapTable = 'tmp/antirobot_botnet_%s_map' % DateToStr(date)
    try:
        MapReduce.runMap(Map(searchOnly), srcTable = 'antirobot_daemon/%s' % DateToStr(date), dstTable = dstMapTable)
        MapReduce.runReduce(Reduce(ipMin, reqMin), srcTable=dstMapTable, dstTable=resultTable)

        FastPrintTable(resultTable)
    finally:
        if not DEBUG:
            MapReduce.dropTable(dstMapTable)
            MapReduce.dropTable(resultTable)


if __name__ == "__main__":
    DEBUG = os.environ.get('DEBUG', False)
    Main()
