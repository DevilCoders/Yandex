import sys
import os
import datetime
import pickle
from collections import deque, namedtuple
from optparse import OptionParser

import yt.wrapper as yt

from antirobot.scripts.utils import filter_yandex
from antirobot.scripts.utils import ip_utils

import rnd_request
import utl

from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from susp_info import SuspInfo

from antirobot.scripts.access_log import RequestYTDict


DEBUG = False
REQ_PER_LOGIN_THREASHOLD = 250

RpsStatItem = namedtuple('RpsStatItem', 'minRps maxRps')

class Rps:
    WINDOW = 16

    def __init__(self):
        self.deq = deque(maxlen=self.WINDOW)

    # timestamp in seconds
    def Next(self, timestamp):
        self.deq.append(timestamp)
        if len(self.deq) == self.WINDOW:
            diff = (self.deq[-1] - self.deq[0])
            return self.WINDOW / diff if diff > 0 else 0
        else:
            return 0

class Map:
    def __init__(self, ipSet):
        self.IpSet = ipSet

    def __call__(self, rec):
        try:
            req = RequestYTDict(rec)
            ipStr = req.ip.strip()
            ipNum = ip_utils.ipStrToNum(ipStr)
            timestamp = float(req.time)
        except:
            return

        if ipNum in self.IpSet:
            dt = datetime.datetime.fromtimestamp(timestamp)

            if filter_yandex.is_yandex(ipNum):
                return

            if not utl.IsSearchReq(req.url, checkMethod = True, method = req.method, checkImagePager = True):
                return

            dt = datetime.datetime.fromtimestamp(timestamp)
            yield {'key': rnd_request.IpDateKey(ipNum, dt), 'timestamp': timestamp}


class Reduce:
    def __init__(self):
        pass

    def __call__(self, key, recs):
        """
            IpDateKey | timestamp | ''
        """
        rps = Rps()
        maxRps = 0
        minRps = None

        for r in recs:
            curRps = rps.Next(float(r['timestamp']))
            if curRps > maxRps:
                maxRps = curRps

            if not minRps or curRps < minRps:
                minRps = curRps

        yield {'key': key['key'], 'minRps': minRps, 'maxRps': maxRps}


class Analyzer(TweakTask):
    NAME = 'high_rps'
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'

    NeedPrepare = True

    def _ReadResult(self, resultTable):
        result = {}

        self.Trace('Reading result table...')
        for r in self.ReadTable(resultTable):
            ipDateKey = r['key']
            minRps = r['minRps']
            maxRps = r['maxRps']

            self.rpsStat[ipDateKey] = RpsStatItem(minRps, maxRps)

        self.Trace('Reading done')

        return result


#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)

        self.rndRequests = rndRequestData
        self.rpsStat = {} # dict: IpDate => rpsStat
        self.reqids = {} # dict: reqid => ipDateKey


    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_high_rps.map') as tmpMap, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_high_rps.res') as tmpResult:

            ipSet = self.rndRequests.GetIpSet()

            srcTables = utl.MakeTablesList('//logs/yandex-access-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd)

            self.RunMap(Map(ipSet), srcTables, tmpMap)
            self.RunSort(tmpMap, ['key', 'timestamp'])
            self.RunReduce(Reduce(), tmpMap, tmpResult, reduce_by='key')

            self._ReadResult(tmpResult)

            self.Trace("End preparing")

        self.SavePrepared(outDir)

    def LoadPrepared(self, dirName):
        self.Trace('Start loading prepared data from %s' % dirName)
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.rpsStat = pickle.load(open(fileName))
        self.Trace('Prepared data loaded')

    def SavePrepared(self, dirName):
        self.Trace('Saving prepared...')
        fileName = os.path.join(dirName, self.STATE_FILE)
        pickle.dump(self.rpsStat, open(fileName, 'w'))
        self.Trace('Saved prepared')

    def PrintStat(self):
        for req in self.rndRequests:
            rpsStatItem = self.rpsStat.get(req.DateKey())
            if rpsStatItem:
                print '\t'.join((req.Raw.reqid, ip_utils.ipNumToStr(req.Raw.ip), str(rpsStatItem.minRps), str(rpsStatItem.maxRps)))

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        rpsStatItem = self.rpsStat.get(req.DateKey())

        if not rpsStatItem:
            return None

        if float(rpsStatItem.maxRps) >= 101.0:
            return SuspInfo(coeff = 1, name = self.NAME, descr = str(rpsStatItem))

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
        for i in sys.stdin:
            fs = i.split('\t')
            reqid = fs[0]
            info  = analyzer.SuspiciousInfo(reqid)
            if info:
                 print '\t'.join([reqid, info])
    else:
        analyzer.Trace('Bad command')


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)

if __name__ == "__main__":
    Main()


