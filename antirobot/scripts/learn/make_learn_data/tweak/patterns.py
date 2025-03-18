import sys
import os
import re
import datetime
from optparse import OptionParser

import yt.wrapper as yt

try:
    from library.python import resource
except:
    resource = None

from antirobot.scripts.utils import filter_yandex
from antirobot.scripts.utils import ip_utils

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak.rnd_request import IpDateKey, IpDateFromKey
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo
from antirobot.scripts.learn.make_learn_data.tweak import utl
from antirobot.scripts.learn.make_learn_data.tweak import state_pickle

from antirobot.scripts.access_log import Request
from antirobot.scripts.access_log import RequestYTDict


PATTERNS_NAME = '/patterns.txt'


def ReadPatterns(patternsPath=None):
    if patternsPath:
        lines = open(patternsPath).readlines()
    elif resource:
        lines = resource.find(PATTERNS_NAME).split('\n')
    else:
        raise Exception("Could not load patterns")

    return [x.strip() for x in lines]


reReqText = re.compile(r'(?:text|query)=([^ &]*)')
def GetRequestText(req):
    m = reReqText.search(req)
    if m:
        return m.group(1)
    else:
        return None

class PatternsChecker:
    def __init__(self, patterns):
        reStr = '|'.join(patterns)
        self.rePatterns = re.compile(reStr)

    def HasPattern(self, str):
        return self.rePatterns.search(str) != None


class Map:
    def __init__(self, ipSet, patternsChecker):
        self.patternsChecker = patternsChecker
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

        if ipNum in self.IpSet:
            if utl.IsSearchReq(req.url, checkMethod = True, method = req.method, checkImagePager = True):
                reqText = GetRequestText(req.url)
                if not reqText:
                    return
                if self.patternsChecker.HasPattern(reqText):
                    dt = datetime.datetime.fromtimestamp(timestamp)
                    yield {'key': IpDateKey(ipStr, dt)}


def Reduce(key, recs):
    """
        'key': ipDateKey
    """
    total = 0
    for r in recs:
        total += 1

    yield {'key': key['key'], 'total': total}


class IpStat(state_pickle.Serializable):
    __slots__ = ['ip', 'date', 'reqsWithPatterns', 'reqHasPattern']
    AttrTypes = {
        'ip': 'int',
        'date': 'date',
        'reqsWithPatterns': 'int',
        'reqHasPattern': 'int',
    }

    def __init__(self, ipDateKey = None):
        self.ip, self.date = IpDateFromKey(ipDateKey) if ipDateKey else (None, None)
        self.reqsWithPatterns = 0
        self.reqHasPattern = 0

    def Update(self, reqsWithPatterns):
        self.reqsWithPatterns += int(reqsWithPatterns)

    def ToStr(self, delim):
        return delim.join(map(str, (ip_utils.ipNumToStr(self.ip), self.reqsWithPatterns, self.reqHasPattern)))

    def __str__(self):
        return self.ToStr('\t')

class Analyzer(TweakTask):
    NAME = 'patterns'
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'

    NeedPrepare = True

    def _ReadResult(self, resultTable):
        result = {}

        def GetIpStat(ipDateKey):
            stat = self.ipStat.get(ipDateKey)
            if not stat:
                stat = IpStat(ipDateKey)
                self.ipStat[ipDateKey] = stat
            return stat

        for r in self.ReadTable(resultTable):
            (ipDateKey, reqsWithPatterns) = (r['key'], r['total'])
            stat = GetIpStat(ipDateKey)
            if stat:
                stat.Update(reqsWithPatterns)

        return result


#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)

        patterns = ReadPatterns(kw.get('patternsPath'))
        self.patternsChecker = PatternsChecker(patterns)

        self.rndRequests = rndRequestData
        self.ipStat = {} # dict: ip => IpStat
        self.reqHasPattern = 0 # req text from rnd_reqdata has pattern

    def UpdateRndReqFull(self, rndReqFullIter):
        for i in rndReqFullIter:
            key = i.DateKey()
            ipStat = self.ipStat.get(key)
            if not ipStat:
                ipStat = IpStat(key)
                self.ipStat[key] = ipStat
            if self.patternsChecker.HasPattern(i.request):
                ipStat.reqHasPattern = 1

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_patt.map') as tmpAccMap, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_patt.res') as tmpResult:

            ipSet = self.rndRequests.GetIpSet()

            srcTables = utl.MakeTablesList('//logs/yandex-access-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd)

            self.RunMap(Map(ipSet, self.patternsChecker), srcTables, tmpAccMap)
            self.RunSort(tmpAccMap, 'key')
            self.RunReduce(Reduce, tmpAccMap, tmpResult, reduce_by='key')

            self._ReadResult(tmpResult)

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
        self.Trace('Saved prepared')

    def PrintStat(self):
        for req in self.rndRequests:
            ipStat = self.ipStat.get(req.DateKey())
            if ipStat:
                print '\t'.join((req.Raw.reqid, str(int(req.suspicious)), str(ipStat)))

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        ipStat = self.ipStat.get(req.DateKey())
        if not ipStat:
            return None

        if ipStat.reqsWithPatterns > 100 and ipStat.reqHasPattern:
            return SuspInfo(coeff = 1, name = self.NAME, descr = ipStat.ToStr(','))

        return None


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)
