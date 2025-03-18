#!/usr/bin/env python

import sys
import os
import re
import datetime
from optparse import OptionParser

from antirobot.scripts.utils import filter_yandex
from antirobot.scripts.utils import ip_utils

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak.rnd_request import IpDateKey, IpDateFromKey
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo
from antirobot.scripts.learn.make_learn_data.tweak import utl
from antirobot.scripts.learn.make_learn_data.tweak import state_pickle

import yt.wrapper as yt
from yt.wrapper import TablePath


THIS_DIR = os.path.dirname( os.path.realpath(__file__))
DEBUG = False
PATTERNS_FILE = THIS_DIR + '/patterns.txt'


class Map:
    def __init__(self, ipSet):
        self.IpSet = ipSet
        self.reReg = re.compile(r'\b(ip|t_tot|ts)=([\d\.]+)')

    def __call__(self, rec):
        keys = {'ip': None, 't_tot': None, 'ts': None}

        for i in self.reReg.finditer(rec['value']):
            keys[i.group(1)] = i.group(2)

        if keys['ip'] != None and keys['t_tot'] != None and keys['ts'] != None:
            try:
                ipNum = ip_utils.ipStrToNum(keys['ip'])
            except:
                return

            if filter_yandex.is_yandex(ipNum):
                return

            if ipNum in self.IpSet:
                try:
                    dt = datetime.datetime.fromtimestamp(int(keys['ts']))
                    yield {'key': IpDateKey(keys['ip'], dt), 'value': keys['t_tot']}
                except:
                    pass


class Reduce:
    def __init__(self, threashold):
        self.threashold = threashold

    def __call__(self, key, recs):
        """
            ipDateKey, '', t_tot
        """
        total = 0
        heavyLoadCount = 0
        for r in recs:
            total += 1
            if float(r['value']) >= self.threashold:
                heavyLoadCount += 1


        yield {'key': key, 'total': total, 'heavy_load_count': heavyLoadCount}


class IpStat(state_pickle.Serializable):
    __slots__ = ["ip", "date", "totalReqs", "highLoadReqs"]

    AttrTypes = {
        'ip': 'int',
        'date': 'date',
        'totalReqs': 'int',
        'highLoadReqs': 'int'
    }

    def __init__(self, ipDateKey=None):
        self.ip, self.date = IpDateFromKey(ipDateKey) if ipDateKey else (None, None)
        self.totalReqs = 0
        self.highLoadReqs = 0


    def ToStr(self, delim = '\t'):
        return delim.join(map(str, (IpDateKey(self.ip, self.date), self.totalReqs, self.highLoadReqs)))

    def __str__(self):
        return self.ToStr('\t')

    def __getstate__(self):
        res  = {}
        for attr in self.__slots__:
            res[attr] = getattr(self, attr)
        return res

    def __setstate__(self, state):
        for attr in self.__slots__:
            setattr(self, attr, state[attr])

class Analyzer(TweakTask):
    NAME = 'hiload'
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'

    NeedPrepare = True

    def _ReadResult(self, resultTable):
        self.Trace("Reading table %s" % resultTable)
        result = {}

        def GetIpStat(ipDateKey):
            stat = self.ipStat.get(ipDateKey)
            if not stat:
                stat = IpStat(ipDateKey)
                self.ipStat[ipDateKey] = stat
            return stat

        for r in self.ReadTable(resultTable):
            ipDateKey = r['key']
            total = r['total']
            highLoad = r['heavy_load_count']
            stat = GetIpStat(ipDateKey)
            if stat:
                stat.totalReqs = int(total)
                stat.highLoadReqs = int(highLoad)

        return result


#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)

        self.rndRequests = rndRequestData
        self.ipStat = {} # dict: ipDateKey => IpStat

    def UpdateRndReqFull(self, rndReqFullIter):
        return
        for i in rndReqFullIter:
            key = i.DateKey()
            ipStat = self.ipStat.get(key)
            if not ipStat:
                ipStat = IpStat(key)
                self.ipStat[key] = ipStat

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing: highload")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_load.map') as tmpMap, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_load.res') as tmpResult:

            ipSet = self.rndRequests.GetIpSet()

            allSrcTables = utl.MakeTablesList('//logs/reqans-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd)

            TABLES_AT_ONCE = 5
            for srcTables in [allSrcTables[i:i+TABLES_AT_ONCE] for i in xrange(0, len(allSrcTables), TABLES_AT_ONCE)]:
                self.RunMap(Map(ipSet), srcTables, TablePath(tmpMap, append=True))

            self.RunSort(tmpMap, 'key')
            self.RunReduce(Reduce(0.8), tmpMap, tmpResult, reduce_by='key')

            self._ReadResult(tmpResult)

            self.Trace("End preparing")

        self.SavePrepared(outDir)

    def LoadPrepared(self, dirName):
        self.Trace('Start loading prepared data from %s' % dirName)
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.ipStat = state_pickle.LoadDict(open(fileName), IpStat)
        else:
            print >>sys.stderr, "'%s' does not exist" % fileName
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
                print '\t'.join((req.Raw.reqid, str(int(req.suspicious)), ipStat.ToStr()))

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        ipStat = self.ipStat.get(req.DateKey())
        if not ipStat:
            return None

        coeff = 1 if req.suspicious else 0.5

        if ipStat.totalReqs > 100 and float(ipStat.highLoadReqs) / ipStat.totalReqs >= 0.15:
            return SuspInfo(coeff = coeff, name = self.NAME, descr = ipStat.ToStr(','))

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
    parser.add_option('', '--patterns', dest='patterns', action='store', type='string', help="path to file with patterns")
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


