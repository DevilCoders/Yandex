#!/usr/bin/env python

import sys
import os
import re
import datetime
from optparse import OptionParser

import yt.wrapper as yt
from yt.wrapper import TablePath

from antirobot.scripts.utils import filter_yandex
from antirobot.scripts.utils import ip_utils

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak.rnd_request import IpDateKey, IpDateFromKey
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo

from antirobot.scripts.access_log import RequestYTDict

from antirobot.scripts.learn.make_learn_data.tweak import state_pickle
from antirobot.scripts.learn.make_learn_data.tweak import utl


DEBUG = False

REQ_NOSEARCH = 0
REQ_SEARCH = 1
REQ_XMLSEARCH = 3
REQ_CYCOUNTER = 4

reReqText = re.compile(r'(?:text|query)=([^ &]*)')
def GetRequestText(req):
    m = reReqText.search(req)
    if m:
        return m.group(1)
    else:
        return None

def GetRequestType(method, req):
    if req.startswith("/xmlsearch"):
        return REQ_XMLSEARCH

    if req.startswith("/cycounter"):
        return REQ_CYCOUNTER

    if not (method == 'GET' and (
            req.startswith("/yandsearch") or
            req.startswith("/msearch") or
            req.startswith("/telsearch") or
            req.startswith("/touchsearch") or
            req.startswith("/largesearch") or
            req.startswith("/schoolsearch") or
            req.startswith("/familysearch") or
            req.startswith("/sitesearch") or
            req.startswith("/search"))):
        return REQ_NOSEARCH

    if req.find('rpt=imagepager') >= 0:
        return REQ_NOSEARCH

    return REQ_SEARCH


# out table: tmpAccMap
class MapByIpAndReqType:
    def __init__(self, ipSet):
        self.IpSet = ipSet

    def __call__(self, rec):
        try:
            req = RequestYTDict(rec)
            ipStr = req.ip.strip()
            ipNum = ip_utils.ipStrToNum(ipStr)
            timestamp = int(req.time)
        except:
            return

        if filter_yandex.is_yandex(ipNum):
            return

        if req.host.find('"images.yandex.') >= 0:
            return

        if ipNum in self.IpSet:
            reqType = GetRequestType(req.method, req.url)
            dt = datetime.datetime.fromtimestamp(timestamp)

            yield {'key': IpDateKey(ipStr, dt), 'type': reqType, 'reqid': req.reqid}

# out table: tmpJoin (0), tmpCounts (1)
def ReduceOnAcc(key, recs):
    """
        ipDate, reqType, reqid
    """

    total = 0
    searchReqs = 0
    xmlReqs = 0
    cycounters = 0
    for r in recs:
        total += 1
        if r['type'] == REQ_SEARCH:
            searchReqs += 1
            yield {'reqid': r['reqid'], 'tag': 1, 'key': key['key'], '@table_index':  0}
        elif r.subkey == str(REQ_XMLSEARCH):
            xmlReqs += 1
        elif r.subkey == str(REQ_CYCOUNTER):
            cycounters += 1

    yield {'key': key, 'total': total, 'search_reqs': searchReqs, 'cycounters': cycounters, 'xml_reqs': xmlReqs, '@table_index': 1}


# out table: tmpJoin
class MapReqidRedir:
    def __init__(self, ipSet):
        self.ipSet = ipSet

    def __call__(self, rec):
        """
            Input: redir_log record
            Output: reqid '2' rediriect_type
        """

        fields = rec['value'].split('@@')

        ipStr = fields[-2]

        try:
            ipNum = ip_utils.ipStrToNum(ipStr)
        except:
            return

        if ipNum in self.ipSet:
            reqid = ''
            redType = '-1'

            for f in fields:
                keyVal = f.split('=')
                if len(keyVal) > 1:
                    if keyVal[0] == 'reqid':
                        reqid = keyVal[1]
                    elif keyVal[0] == 'at':
                        redType = keyVal[1]

            if reqid:
                yield {'reqid': reqid, 'tag': 2, 'redir_type': redType}

# out table: tmpResult
def ReduceClicks1(key, recs):
    """
        Input: reqid joinVal someVal
        Output: ip trgetInfo
    """

    clickCount = 0
    ipDateKey = None
    for r in recs:
        if r['tag'] == 1: # from access_log
            if ipDateKey:
                return # something wrong - there should be no more than one reqs with '1' subkey

            ipDateKey = r['key']
        elif r['tag'] == 2:
            clickCount += 1

    if ipDateKey:
        yield {'key': ipDateKey, 'count': clickCount}

def ReduceClicks2(key, recs):
    clickCount = 0
    for r in recs:
        clickCount += int(r['count'])

    yield {'key': key['key'], 'count': clickCount}


class IpStat(state_pickle.Serializable):
    __slots__ = ["ip", "date", "totalReqs", "clickCount", "searchReqs", "cycounters", "xmlSearchReqs"]

    AttrTypes = {
        "ip": 'int',
        'date': 'date',
        'totalReqs': 'int',
        'clickCount': 'int',
        'searchReqs': 'int',
        'xmlSearchReqs': 'int',
        'cycounters': 'int',
    }

    def __init__(self, ipDateKey = None):
        self.ip, self.date = IpDateFromKey(ipDateKey) if ipDateKey else (None, None)
        self.totalReqs = 0 # total search request count
        self.clickCount = 0    # total click count
        self.searchReqs = 0
        self.cycounters = 0
        self.xmlSearchReqs = 0

    def UpdateClicks(self, clicks):
        self.clickCount += int(clicks)

    def UpdateReqTypes(self, totalReqs, searchReqs, cycounters, xmlReqs):
        self.totalReqs += int(totalReqs)
        self.searchReqs += int(searchReqs)
        self.cycounters += int(cycounters)
        self.xmlSearchReqs = int(xmlReqs)

    def __str__(self):
        return '\t'.join(map(str, (ip_utils.ipNumToStr(self.ip), self.totalReqs, self.searchReqs, self.cycounters, self.xmlSearchReqs, self.clickCount)))


class Analyzer(TweakTask):
    NAME = 'clicks'
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'

    NeedPrepare = True
    weight = 4

    def _ReadResult(self, clicksTable, reqTypesTable):
        result = {}

        def GetIpStat(ipDateKey):
            stat = self.ipStat.get(ipDateKey)
            if not stat:
                stat = IpStat(ipDateKey)
                self.ipStat[ipDateKey] = stat
            return stat

        for r in self.ReadTable(clicksTable):
            ip, clickCount = r['key'], r['count']
            stat = GetIpStat(ip)
            if stat:
                stat.UpdateClicks(clickCount)

        for r in self.ReadTable(reqTypesTable):
            ip = r.key
            total, searchReqs, cycounters, xmlSearchReqs = r['total'], r['search_reqs'], r['cycounters'], r['xml_reqs']
            stat = GetIpStat(ip)
            if stat:
                stat.UpdateReqTypes(total, searchReqs, cycounters, xmlSearchReqs)

        return result


#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)
        self.rndRequests = rndRequestData
        self.ipStat = {} # dict: ip => IpStat

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_clicks1.map') as tmpAccMap, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_req_counts.map') as tmpCounts, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_clicks1.red') as tmpJoin, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_clicks.res1') as tmpResult1, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_clicks.res2') as tmpResult2:

            ipSet = self.rndRequests.GetIpSet()

            srcTables = utl.MakeTablesList('//logs/yandex-access-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd)

            self.RunMap(MapByIpAndReqType(ipSet), srcTables, tmpAccMap)
            self.RunSort(tmpAccMap, 'key')

            self.RunReduce(ReduceOnAcc, tmpAccMap, [tmpJoin, tmpCounts], reduce_by='key')

            self.Trace('Performing map on redir_log...')
            self.RunMap(MapReqidRedir(ipSet), utl.MakeTablesList('//logs/redir-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd),
                            TablePath(tmpJoin, append=True))
            self.RunSort(tmpJoin, 'reqid')

            self.Trace('Performing reduce1 operation...')
            self.RunReduce(ReduceClicks1, tmpJoin, tmpResult1, reduce_by='reqid')
            self.RunSort(tmpResult1, 'key')
            self.Trace('Performing reduce2 operation...')
            self.RunReduce(ReduceClicks2, tmpResult1, tmpResult2, reduce_by='key')

            self.Trace('Reading result...')
            self._ReadResult(tmpResult2, tmpCounts)

            self.Trace('Dumping tables stat...')
            self.Trace("End preparing")
            self.SavePrepared(outDir)


    def LoadPrepared(self, dirName):
        self.Trace('Start loading prepared data from %s' % dirName)
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.ipStat = state_pickle.LoadDict(open(fileName), IpStat)
        self.Trace('Prepared data loaded')

    def SavePrepared(self, dirName):
        self.Trace('Saving prepared...')
        fileName = os.path.join(dirName, self.STATE_FILE)
        state_pickle.DumpDict(self.ipStat, open(fileName, 'w'))
        self.Trace('Saving done')

    def PrintStat(self):
        for req in self.rndRequests:
            ipDateKey = req.DateKey()
            ipStat = self.ipStat.get(ipDateKey)
            if ipStat:
                print '\t'.join((req.reqid, str(int(req.suspicious)), str(ipStat)))

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        ipDateKey = req.DateKey()
        ipStat = self.ipStat.get(ipDateKey)
        if not ipStat:
            return None

        if ipStat.totalReqs > 100 and ipStat.xmlSearchReqs == 0 and ipStat.clickCount == 0 and ipStat.cycounters == 0:
            descr = ','.join((str(ipStat.totalReqs), str(ipStat.searchReqs), str(ipStat.xmlSearchReqs), str(ipStat.clickCount)))
            return SuspInfo(coeff = 1, name = self.NAME, descr = descr)

        return None


def OptParser():
    parser = OptionParser(
'''Usage: %prog cmd [options]
where cmd can be:
    prepare     long-initial running calculation
    final       load prepared data and print it
''')
    parser.add_option('', '--rndreq', dest='rndreqFile', action='store', type='string', help="path to rnd_reqdata")
    parser.add_option('', '--prepared', dest='prepared', action='store', type='string', help="path to prepared data")
    return parser

def Main():
    global DEBUG

    DEBUG = bool(os.getenv('DEBUG'))

    optParser = OptParser()
    (opts, args) = optParser.parse_args()

    if not args or not opts.rndreqFile or args[0] not in ['prepare', 'final', 'check']:
        optParser.print_usage()
        sys.exit(2)

    cmd = args[0]

    rndReqData = rnd_request.RndRequestData.Load(open(opts.rndreqFile))
    outDir = opts.prepared if opts.prepared else '.'

    entrObj = Analyzer(rndReqData)

    if cmd == 'prepare':
        entrObj.Trace('Preparing...')
        entrObj.Prepare(outDir)
        entrObj.Trace('All done.')
    elif cmd == 'final':
        entrObj.LoadPrepared(outDir)
        entrObj.PrintStat()
        for i in entrObj.ipStat.iterkeys():
            print i

    elif cmd == 'check':
        entrObj.LoadPrepared(outDir)
        for i in sys.stdin:
            fs = i.split('\t')
            reqid = fs[0]
            info  = entrObj.SuspiciousInfo(reqid)
            if info:
                 print '\t'.join([reqid] +  map(str, info))

    else:
        entrObj.Trace('Bad command')


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)

if __name__ == "__main__":
    Main()


