#!/usr/bin/env python

import sys
import os
import datetime
import pickle
from optparse import OptionParser
import re

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo

import yt.wrapper as yt

DEBUG = False

def YUidKey(yandexuid, dat):
    return '%s:%s' % (yandexuid, dat.strftime('%Y%m%d'))

def MakeTablesList(dayStart, dayEnd):
    res = []
    i = dayStart
    while i <= dayEnd:
        res.append('//user_sessions/preprod/pub/search/daily/%s/frauds' % i.strftime('%Y-%m-%d'))
        i += datetime.timedelta(days=1)

    return res

class Map:
    def __init__(self, yandexUidSet):
        self.yandexUidSet = yandexUidSet
        self.reSuggest = re.compile('type=SUGGEST')

    def __call__(self, rec):
        if not rec['key'].startswith('y'):
            return

        uid = rec['key'].split('y')[1]

        if uid:
            ykey = YUidKey(uid, datetime.datetime.fromtimestamp(int(rec['subkey'])).date())
            suggest = 1 if self.reSuggest.search(rec['value']) else 0

            if uid in self.yandexUidSet:
                yield {'key': ykey, 'suggest': suggest}


def Reduce(key, recs):
    for r in recs:
        if r['suggest']:
            return

    yield {'key': key['key']}


class Analyzer(TweakTask):
    NAME = 'frauds'
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'

    NeedPrepare = True

    def _ReadResult(self, resultTable):
        self.Trace("Reading table %s" % resultTable)

        for r in self.ReadTable(resultTable):
            self.stat.add(r['key'])


#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)

        self.rndRequests = rndRequestData
        self.yuids = {} # reqid => yuid
        self.stat = set() # bad yuids from mapreduce stored as YUidKey
        self.rndUids = set()

    def UpdateRndReqFull(self, rndReqFullIter):
        self.uids = {}
        for req in rndReqFullIter:
            yandexuid = req.cookies.get('yandexuid')
            if yandexuid:
                self.yuids[req.Raw.reqid] = yandexuid.value
                self.rndUids.add(yandexuid.value)

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_frauds.map') as tmpMap, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_frauds.res') as tmpResult:

            srcTables = MakeTablesList(self.rndRequests.dateStart, self.rndRequests.dateEnd)

            self.RunMap(Map(self.rndUids), srcTables, tmpMap)
            self.RunSort(tmpMap, 'key')
            self.RunReduce(Reduce, tmpMap, tmpResult, reduce_by='key')

            self._ReadResult(tmpResult)

            self.Trace("End preparing")

        self.SavePrepared(outDir)

    def LoadPrepared(self, dirName):
        self.Trace('Start loading prepared data from %s' % dirName)
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.stat = pickle.load(open(fileName))
        self.Trace('Prepared data loaded')
        print >>sys.stderr, len(self.stat)

    def SavePrepared(self, dirName):
        self.Trace('Saving prepared...')
        fileName = os.path.join(dirName, self.STATE_FILE)
        pickle.dump(self.stat, open(fileName, 'w'))
        self.Trace('Saved prepared')

    def PrintStat(self):
        for req in self.rndRequests:
            yuid = self.yuids.get(req.Raw.reqid)
            if yuid:
                uidKey = YUidKey(yuid, req.timestamp)
                if uidKey in self.stat:
                    print '\t'.join((req.Raw.reqid, str(int(req.suspicious)), yuid))

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        yuid = self.yuids.get(req.Raw.reqid)
        if yuid:
            uidKey = YUidKey(yuid, req.timestamp)
            if uidKey in self.stat:
                return SuspInfo(coeff = 1, name = self.NAME, descr = '%s' % yuid)

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

    analyzer = Analyzer(rndReqData)
    print >>sys.stderr, "Updating RndReqFull..."
    analyzer.UpdateRndReqFull(rnd_request.GetRndReqFullIter(opts.rndreqFile))

    if cmd == 'prepare':
        analyzer.Trace('Preparing..')
        analyzer.Prepare(outDir)
        analyzer.Trace('All done.')
    elif cmd == 'final':
        analyzer.LoadPrepared(outDir)
        analyzer.PrintStat()
    elif cmd == 'check':
        analyzer.LoadPrepared(outDir)
#for i in sys.stdin:
        for r in analyzer.rndRequests:
            info  = analyzer.SuspiciousInfo(r.Raw.reqid)
            if info:
                 print '\t'.join((r.Raw.reqid, str(info.weight), info.name, info.descr))
    else:
        analyzer.Trace('Bad command')


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)

if __name__ == "__main__":
    Main()
