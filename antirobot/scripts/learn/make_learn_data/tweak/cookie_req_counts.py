#!/usr/bin/env python
# count request from yandexuid per day, suspicious if more than some threshold

import sys
import os
import pickle
import datetime
from optparse import OptionParser

from antirobot.scripts.utils import filter_yandex
from antirobot.scripts.utils import ip_utils

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo
from antirobot.scripts.learn.make_learn_data.tweak import utl

from antirobot.scripts.access_log import RequestYTDict

import yt.wrapper as yt
from yt.wrapper import TablePath

DEBUG = bool(os.getenv('DEBUG'))

class MapOperation:
    def __init__(self, cookieFunc):
        self.debug = DEBUG
        self.cookieFunc = cookieFunc

    def __call__(self, rec):
        try:
            req = RequestYTDict(rec)
            timestamp = int(req.time)
            ipStr = req.ip.strip()
            ipNum = ip_utils.ipStrToNum(ipStr)
        except:
            return

        dt = datetime.datetime.fromtimestamp(timestamp)

        if filter_yandex.is_yandex(ipNum):
            return

        if not utl.IsSearchReq(req.url, checkMethod = False, checkImagePager = True):
            return

        if req.yandexuid:
            yield {'cookie': req.yandexuid, 'value': 1}


class ReduceOperation:
    def __init__(self):
        pass

    def __call__(self, key, recs):
        totals = 0

        for rec in recs:
            totals += rec['value']

        yield {'cookie': key['cookie'], 'total': totals}


class Analyzer(TweakTask):
    NAME = 'cookie_req_counts'
    NeedPrepare = True
    STATE_FILE = 'prepared.state'
    REQUEST_PER_DAY_MIN_THRESHOLD = 100
    REQUEST_PER_DAY_ACTUAL_THRESHOLD = 500

    Weight = 4

    def _ReadResult(self, tbl):
        self.cookies = {}

        self.Trace("Start reading result table")
        for r in self.ReadTable(tbl):
            (cookie, count) = (r['cookie'], r['total'])

            if self.SIMULATE or int(count) / self.daysCount >= self.REQUEST_PER_DAY_MIN_THRESHOLD:
                self.cookies[cookie] = int(count)


#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)
        self.rndRequests = rndRequestData
        self.cookies = {} # yandexuid => count
        self.daysCount = (rndRequestData.dateEnd - rndRequestData.dateStart).days + 1
        self.rndReqCookies = {} # reqid => yandexuid

    def UpdateRndReqFull(self, rndReqFullIter):
        for req in rndReqFullIter:
            yandexuid = req.cookies.get('yandexuid')
            if yandexuid:
                self.rndReqCookies[req.Raw.reqid] = yandexuid.value

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        srcTables = utl.MakeTablesList('//logs/yandex-access-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd)

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_cookie_count.map') as mapTable, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_cookie_count.red') as redTable:

            self.RunMap(MapOperation(utl.GetYandexUid), srcTables, mapTable)
            self.RunSort(mapTable, 'cookie')
            self.Trace("Map done")

            self.Trace("Run reduce")
            self.RunReduce(ReduceOperation(), mapTable, redTable, reduce_by='cookie')
            self.Trace("Reduce done")

            self._ReadResult(redTable)
            self.Trace("End preparing")

        self.SavePrepared(outDir)

    def LoadPrepared(self, dirName):
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.cookies = pickle.load(open(fileName))

    def SavePrepared(self, dirName):
        self.Trace("Saving prepared...")
        fileName = os.path.join(dirName, self.STATE_FILE)
        pickle.dump(self.cookies, open(fileName, 'w'))
        self.Trace('Saved prepared')

    def PrintStat(self):
        for req in self.rndRequests:
            yandexuid = self.rndReqCookies.get(req.reqid)
            count = self.cookies.get(yandexuid, 0)
            if yandexuid:
                print '\t'.join((req.Raw.reqid, yandexuid, str(count), str(self.daysCount), str(count / self.daysCount)))
#for cook in self.cookies

    def SuspiciousInfo(self, reqid):
        rndReq = self.rndRequests.GetByReqid(reqid)
        if not rndReq:
            return None

        yandexuid = self.rndReqCookies.get(rndReq.Raw.reqid)
        if yandexuid and self.cookies.get(yandexuid, 0) / self.daysCount >= self.REQUEST_PER_DAY_ACTUAL_THRESHOLD:
            return SuspInfo(coeff = 1, name = self.NAME, descr = '%d' % self.cookies[yandexuid])

        return None


def OptParser():
    parser = OptionParser(
'''Usage: %prog cmd [options]
where cmd can be:
    prepare     long-running initial calculation
    final       load prepared data and print it
''')
    parser.add_option('', '--rndreq', dest='rndreqFile', action='store', type='string', help="path to rnd_reqdata")
    parser.add_option('', '--prepared', dest='prepared', action='store', type='string', help="path to prepared data")
    return parser

def Main():
    optParser = OptParser()
    (opts, args) = optParser.parse_args()

    if not args or not opts.rndreqFile or args[0] not in ['prepare', 'final']:
        optParser.print_usage()
        sys.exit(2)

    cmd = args[0]

    rndReqdataFile = opts.rndreqFile
    rndReqData = rnd_request.RndRequestData.Load(open(rndReqdataFile))
    outDir = opts.prepared if opts.prepared else '.'

    analyzer = Analyzer(rndReqData)
    analyzer.UpdateRndReqFull(rnd_request.GetRndReqFullIter(rndReqdataFile))

    if cmd == 'prepare':
        analyzer.Trace("Preparing..")
        analyzer.Prepare(outDir)
        analyzer.SavePrepared(outDir)
        analyzer.Trace("All done.")
    elif cmd == 'final':
        analyzer.LoadPrepared(outDir)
        analyzer.UpdateRndReqFull(rnd_request.GetRndReqFullIter(rndReqdataFile))
        analyzer.PrintStat()
    else:
        analyzer.Trace('Bad command')


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)

if __name__ == "__main__":
    Main()


