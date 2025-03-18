#!/usr/bin/env python

import sys
import os

import datetime
import pickle
from optparse import OptionParser

from antirobot.scripts.utils import filter_yandex
from antirobot.scripts.utils import ip_utils
from antirobot.scripts.access_log import RequestYTDict

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak.rnd_request import IpDateFromKey
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo
from antirobot.scripts.learn.make_learn_data.tweak import utl

import yt.wrapper as yt


DEBUG = False
REQ_PER_LOGIN_THREASHOLD = 250

def LoginDateKey(yaLogin, dat):
    return '%s:%s' % (yaLogin, dat.strftime('%Y-%m-%d'))


class Map:
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

        if ipNum in self.IpSet:
            dt = datetime.datetime.fromtimestamp(timestamp)

            if filter_yandex.is_yandex(ipNum):
                return

            if not utl.IsSearchReq(req.url, checkMethod = True, method = req.method, checkImagePager = True):
                return

            yaLogin = req.GetYandexLogin()
            if not yaLogin:
                return

            dt = datetime.datetime.fromtimestamp(timestamp)
            yield {'key': LoginDateKey(yaLogin, dt)}


class Reduce:

    def __init__(self):
        pass

    def __call__(self, key, recs):
        """
            'key': loginDateKey
        """
        total = 0
        for r in recs:
            total += 1

        yield {'key': key['key'], 'total': total}


class YaLoginStat:
    def __init__(self, ipDateKey):
        self.ip, self.date = IpDateFromKey(ipDateKey)
        self.averageLoad = 0
        self.maxLoad = 0
        self.minLoad = 0


class Analyzer(TweakTask):
    NAME = 'req_login'
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'

    NeedPrepare = True

    def _ReadResult(self, resultTable):
        result = {}

        self.Trace('Reading result table...')
        for r in self.ReadTable(resultTable):
            (loginDateKey, count) = (r.key, r.value)
            self.loginReqCount[loginDateKey] = int(count)

        self.Trace('Reading done')

        return result


#public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)

        self.rndRequests = rndRequestData
        self.loginReqCount = {} # dict: yandex_login:date => YaLoginStat
        self.reqidLogin = {} # dict: reqid => login

    def UpdateRndReqFull(self, rndReqFullIter):
        for i in rndReqFullIter:
            login = i.cookies.get('yandex_login')
            if login:
                self.reqidLogin[i.Raw.reqid] = login.value

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_req_login.map') as tmpMap, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_req_login.res') as tmpResult:

            ipSet = self.rndRequests.GetIpSet()

            srcTables = utl.MakeTablesList('//logs/yandex-access-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd)

            self.RunMap(Map(ipSet), srcTables, tmpMap)
            self.RunSort(tmpMap, 'key')
            self.RunReduce(Reduce(), tmpMap, tmpResult, reduce_by='key')
            self._ReadResult(tmpResult)

            self.Trace("End preparing")

        self.SavePrepared(outDir)

    def LoadPrepared(self, dirName):
        self.Trace('Start loading prepared data from %s' % dirName)
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.loginReqCount = pickle.load(open(fileName))
        self.Trace('Prepared data loaded')

    def SavePrepared(self, dirName):
        self.Trace('Saving prepared...')
        fileName = os.path.join(dirName, self.STATE_FILE)
        pickle.dump(self.loginReqCount, open(fileName, 'w'))
        self.Trace('Saved prepared')

    def PrintStat(self):
        for req in self.rndRequests:
            login = self.reqidLogin.get(req.Raw.reqid)
            count = 0
            if login:
                count = self.loginReqCount.get(LoginDateKey(login, req.timestamp), 0)
            else:
                login = ''
            print '\t'.join((req.Raw.reqid, str(int(req.suspicious)), login, str(count)))

    def SuspiciousInfo(self, reqid):
        req = self.rndRequests.GetByReqid(reqid)
        if not req:
            return None

        login = self.reqidLogin.get(req.Raw.reqid)
        if not login:
            return

        count = int(self.loginReqCount.get(LoginDateKey(login, req.timestamp), 0))

        if count >= REQ_PER_LOGIN_THREASHOLD:
            return SuspInfo(coeff = 1, name = self.NAME, descr = str(count))

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


