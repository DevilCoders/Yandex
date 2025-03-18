#!/usr/bin/env python

import sys
import os
import math
import re
import datetime
import urllib
import pickle
from collections import namedtuple
from optparse import OptionParser

from antirobot.scripts.utils import filter_yandex
from antirobot.scripts.utils import ip_utils

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak.rnd_request import IpDateKey, IpDateFromKey
from antirobot.scripts.learn.make_learn_data.tweak import req_words
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo
from antirobot.scripts.learn.make_learn_data.tweak import utl

from antirobot.scripts.access_log.request import RequestYTDict

import yt.wrapper as yt

DEBUG = False
EVERY_REQUEST_SUBST = '2e0e677b_518d_41e2_95fe_10b28478e5de'

reReqText = re.compile(r'(?:text|query)=([^ &]*)')
def GetRequestText(req):
    m = reReqText.search(req)
    if m:
        return m.group(1)
    else:
        return None

class MapEventSearch:
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

        ipC = ip_utils.ipClassC(ipNum)
        ipB = ip_utils.ipClassB(ipNum)

        if ipNum in self.IpSet or ipC in self.IpSet or ipB in self.IpSet:
            dt = datetime.datetime.fromtimestamp(timestamp)

            if filter_yandex.is_yandex(ipNum):
                return

            if not utl.IsSearchReq(req.url, checkMethod = True, method = req.method, checkImagePager = True):
                return

            reqText = GetRequestText(req.url)
            words = req_words.SplitReqWords(reqText)

            yield {'key': IpDateKey(ipNum, dt), 'value': EVERY_REQUEST_SUBST}
#            yield Record(IpDateKey(ipC, dt), '', EVERY_REQUEST_SUBST)
#            yield Record(IpDateKey(ipB, dt), '', EVERY_REQUEST_SUBST)

            for w in words:
                w = urllib.quote(w)
                #yield Record(IpDateKey(ipNum, dt), '', w + '\t' + '\t'.join([str(reqText), fields[-3]]))
                yield {'key': IpDateKey(ipNum, dt), 'value': w}
#                yield Record(IpDateKey(ipC, dt), '', w)
#                yield Record(IpDateKey(ipB, dt), '', w)


class ReduceOp:
    def __init__(self, ipSet):
        self.IpSet = ipSet

    def __call__(self, key, recs):
        wordMap = {}
        totalRequests = 0
        totalWords = 0
        (ip, date) = IpDateFromKey(key['key'])

        if not ip in self.IpSet:
            return

        for rec in recs:
            #word = rec.value.strip().split('\t')[0]
            word = rec['value'].strip()

            if word == EVERY_REQUEST_SUBST:
                totalRequests += 1
            else:
                totalWords += 1
                wordMap[word] = wordMap.get(word, 0) + 1

        entrRequests = - reduce(lambda x, y: x + float(y) / totalRequests * math.log(float(y) / totalRequests, 2), wordMap.itervalues(), 0)
        entrWords = - reduce(lambda x, y: x + float(y) / totalWords * math.log(float(y) / totalWords, 2), wordMap.itervalues(), 0)
        emptyCount = wordMap.get(req_words.EMPTY_REQUEST_SUBST, 0)

        yield {'key': key['key'], 'value': '\t'.join(str(x) for x in (totalRequests, totalWords, emptyCount, entrRequests, entrWords))}


class Stat(namedtuple('Stat', 'ip date totalRequests totalWords empty entropyRequests entropyWords')):
    def IpDateKey(self):
        return IpDateKey(self.ip, self.date)

    def ToString(self, delim):
        return delim.join((str(self.totalRequests), str(self.totalWords), str(self.empty),
            '%0.3f' % self.entropyRequests, '%0.3f' % self.entropyWords,
            '%0.3f' % self.EntropyRequestsNorm(), '%0.3f' % self.EntropyWordsNorm()))

    @staticmethod
    def Default():
        return Stat(None, None, 0, 0, 0, 0.0, 0.0)

    @staticmethod
    def FromFields(fs):
        (ip, date) = IpDateFromKey(fs[0])
        return Stat(ip, date, int(fs[1]), int(fs[2]), int(fs[3]), float(fs[4]), float(fs[5]))

    def EntropyRequestsNorm(self):
        try:
            return self.entropyRequests / math.log(self.totalRequests, 2)
        except:
            return -1

    def EntropyWordsNorm(self):
        try:
            return self.entropyWords / math.log(self.totalWords, 2)
        except:
            return -1

class ReqidStat:
    def __init__(self, reqid):
        self.reqid = reqid
        self.statIp = Stat.Default()
        #self.statIpC = Stat.Default()
        #self.statIpB = Stat.Default()

    def ReportToString(self, rndReqObj, delim = '\t'):
        return delim.join((self.reqid, str(int(rndReqObj.suspicious)), self.GetStatsStr(delim)))

    def GetStatsStr(self, delim):
        return delim.join((
                self.statIp.ToString(delim),
                #self.statIpC.ToString(delim),
                #self.statIpB.ToString(delim),
               ))

class Analyzer(TweakTask):
    NAME = 'req_entropy'
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'

    suspNets = set()

    NeedPrepare = True
    Weight = 8

    def _Map(self, dstTableName, ipSet):
        self.Trace('Performing map operation...')
        srcTables = utl.MakeTablesList('//logs/yandex-access-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd)

        self.RunMap(MapEventSearch(ipSet), srcTables, dstTableName)
        self.RunSort(dstTableName, 'key')
        self.Trace('Map done')

    def _ReduceAll(self, srcTable, dstTable, ipSet):
        self.Trace('Performing reduce operation...')
        self.RunReduce(ReduceOp(ipSet), srcTable, dstTable, reduce_by='key')
        self.RunSort(dstTable, 'key')
        self.Trace('Reduce done')

    def _ReadResult(self, resultMrTable):
        result = {}

        self.Trace('Just before reading from process\'s stdout')
        for r in self.ReadTable(resultMrTable):
            fs = [r.key] + r.value.strip().split('\t')

            result[r.key] = Stat.FromFields(fs)

        self.Trace('Read table %s, result count = %d ' % (resultMrTable, len(result)))
        return result

    def _Compose(self, mrResult):
        self.Trace('Start compose')
        for req in self.rndRequests:
            reqidStat = ReqidStat(req.Raw.reqid)

            reqidStat.statIp = mrResult.get(req.DateKey(), Stat.Default())
            #reqidStat.statIpC = mrResult.get(req.DateKeyClassC(), Stat.Default())
            #reqidStat.statIpB = mrResult.get(req.DateKeyClassB(), Stat.Default())

            self.stat[req.Raw.reqid] = reqidStat
        self.Trace('Compose done')

#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)
        self.rndRequests = rndRequestData
        self.stat = {} # dict: reqid => ReqidStat

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_req_entr.map') as mapTableObj, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_req_entr.red') as redTableObj:

            ipSet = self.rndRequests.GetIpSet()

            self._Map(mapTableObj, ipSet)
            self._ReduceAll(mapTableObj, redTableObj, ipSet)

            results = self._ReadResult(redTableObj)
            self._Compose(results)

            self.Trace("End preparing")

        self.SavePrepared(outDir)

    def LoadPrepared(self, dirName):
        self.Trace('Start loading prepared data from %s' % dirName)
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.stat = pickle.load(open(fileName))
        self.Trace('Prepared data loaded')

    def SavePrepared(self, dirName):
        self.Trace('Saving prepared...')
        fileName = os.path.join(dirName, self.STATE_FILE)
        pickle.dump(self.stat, open(fileName, 'w'))
        self.Trace('Saved prepared')

    def PrintStat(self):
        for req in self.rndRequests:
            reqidStat = self.stat.get(req.Raw.reqid)
            if reqidStat:
                print reqidStat.ReportToString(req)

    def SuspiciousInfo(self, reqid):
        rndReq = self.rndRequests.GetByReqid(reqid)
        if not rndReq:
            return None

        reqidStat = self.stat.get(reqid)
        if not reqidStat or reqidStat.statIp.totalRequests < 20:
            return None

        entropyRequestsNorm = reqidStat.statIp.EntropyRequestsNorm()
        entropyWordsNorm = reqidStat.statIp.EntropyWordsNorm()

        if entropyRequestsNorm == -1 or entropyWordsNorm == -1:
            return None

        if (entropyRequestsNorm < 0.01 or entropyRequestsNorm > 4.0 or
             entropyWordsNorm < 0.2 or entropyWordsNorm > 0.9):
            coeff = 2 if rndReq.suspicious else 1
            return SuspInfo(coeff = coeff, name = self.NAME, descr = reqidStat.GetStatsStr(','))

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

    if not args or not opts.rndreqFile or args[0] not in ['prepare', 'final']:
        optParser.print_usage()
        sys.exit(2)

    cmd = args[0]

    rndReqData = rnd_request.RndRequestData.Load(open(opts.rndreqFile))
    outDir = opts.prepared if opts.prepared else '.'

    entrObj = Analyzer(rndReqData)

    if cmd == 'prepare':
        entrObj.Trace('Preparing..')
        entrObj.Prepare(outDir)
        entrObj.Trace('All done.')
    elif cmd == 'final':
        entrObj.LoadPrepared(outDir)
        entrObj.PrintStat()
    else:
        entrObj.Trace('Bad command')


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)

if __name__ == "__main__":
    Main()


