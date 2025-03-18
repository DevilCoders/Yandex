#!/usr/bin/env python
# count number of requests with unique spravkas and fuids from ip and subnets

import sys
import os


import datetime
import pickle
from optparse import OptionParser
import hashlib

from antirobot.scripts.utils import filter_yandex
from antirobot.scripts.utils import ip_utils

from antirobot.scripts.learn.make_learn_data.tweak import rnd_request
from antirobot.scripts.learn.make_learn_data.tweak.rnd_request import IpWeekKey, IpWeekFromKey
from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo
from antirobot.scripts.learn.make_learn_data.tweak import utl

from antirobot.scripts.access_log import RequestYTDict

import yt.wrapper as yt


DEBUG = False

def ConvertBadFuidIfNeed(fuid):
    if len(fuid) == 0:
        return fuid

    if len(fuid) != 16:
        md5 = hashlib.md5()
        md5.update(fuid)
        return md5.hexdigest()

    return fuid

class MapEventSearch:
    def __init__(self, ipSet, moretables = False):
        self.IpSet = ipSet
        self.debug = False
        self.moretables = moretables

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

            if not utl.IsSearchReq(req.url, checkMethod = False, checkImagePager = False):
                return

            fuid = ConvertBadFuidIfNeed(req.fuid, True)

            yield {'key': IpWeekKey(ipNum, dt), 'id': fuid, '@table_index': 0}
            yield {'key': IpWeekKey(ipC, dt), 'id': fuid, '@table_index': 0}
            yield {'key': IpWeekKey(ipB, dt), 'id': fuid, '@table_index': 0}

            if self.moretables:
                spravka = GetSpravka(req.GetSpravkaStr())
                login = req.GetYandexLogin()

                yield {'key': IpWeekKey(ipNum, dt), 'id': spravka, '@table_index': 1}
                yield {'key': IpWeekKey(ipC, dt), 'id': spravka, '@table_index': 1}
                yield {'key': IpWeekKey(ipB, dt), 'id': spravka, '@table_index': 1}

                yield {'key': IpWeekKey(ipNum, dt), 'id': login, '@table_index': 2}
                yield {'key': IpWeekKey(ipC, dt), 'id': login, '@table_index': 2}
                yield {'key': IpWeekKey(ipB, dt), 'id': login, '@table_index': 2}


class ReduceOp:
    def __init__(self, ipSet):
        self.IpSet = ipSet

    def __call__(self, key, recs):
        sprSet = set()
        total = 0
        empty = 0
        (ip, week) = IpWeekFromKey(key)

        if not ip in self.IpSet:
            return

        for rec in recs:
            total += 1
            spr = rec['id']
            if spr != '':
                sprSet.add(spr)
            else:
                empty += 1


        yield {'key': key['key'], 'total': total, 'uniq': len(sprSet), 'empty': empty}

class Stat(object):
    __slots__ = ["week", "ip", "total", "uniq", "empty"]

    def __init__(self, **kw):
        self.week = kw.get('week', None)
        self.ip = kw.get('ip', None) # numeric ip
        self.total = kw.get('total', 0)
        self.uniq = kw.get('uniq', 0)
        self.empty = kw.get('empty', 0)

    def __getstate__(self):
        res  = {}
        for attr in self.__slots__:
            res[attr] = getattr(self, attr)
        return res

    def __setstate__(self, state):
        for attr in self.__slots__:
            setattr(self, attr, state[attr])


class ReqidStat(object):
    __slots__ = ["reqid", "sprStatIp", "sprStatIpC", "sprStatIpB",
                "flashStatIp", "flashStatIpC", "flashStatIpB",
                "loginStatIp", "loginStatIpC", "loginStatIpB"]

    def __init__(self, reqid):
        self.reqid = reqid
        self.sprStatIp = Stat()
        self.sprStatIpC = Stat()
        self.sprStatIpB = Stat()

        self.flashStatIp = Stat()
        self.flashStatIpC = Stat()
        self.flashStatIpB = Stat()

        self.loginStatIp = Stat()
        self.loginStatIpC = Stat()
        self.loginStatIpB = Stat()

    def __getstate__(self):
        res  = {}
        for attr in self.__slots__:
            res[attr] = getattr(self, attr)
        return res

    def __setstate__(self, state):
        for attr in self.__slots__:
            setattr(self, attr, state[attr])

    def ReportToString(self, suspicious, badSubnet, delim = '\t'):
        return delim.join((self.reqid, str(int(suspicious)), str(int(badSubnet)))) + delim + self.GetStatsStr(delim);

    def GetStatsStr(self, delim):
        return delim.join(
                (str(self.sprStatIp.total), str(self.sprStatIp.uniq), str(self.sprStatIp.empty),
                str(self.sprStatIpC.total), str(self.sprStatIpC.uniq), str(self.sprStatIpC.empty),
                str(self.sprStatIpB.total), str(self.sprStatIpB.uniq), str(self.sprStatIpB.empty),

                str(self.flashStatIp.total), str(self.flashStatIp.uniq), str(self.flashStatIp.empty),
                str(self.flashStatIpC.total), str(self.flashStatIpC.uniq), str(self.flashStatIpC.empty),
                str(self.flashStatIpB.total), str(self.flashStatIpB.uniq), str(self.flashStatIpB.empty),

                str(self.loginStatIp.total), str(self.loginStatIp.uniq), str(self.loginStatIp.empty),
                str(self.loginStatIpC.total), str(self.loginStatIpC.uniq), str(self.loginStatIpC.empty),
                str(self.loginStatIpB.total), str(self.loginStatIpB.uniq), str(self.loginStatIpB.empty),
                )
               )

    def GetShortStatsStr(self, delim):
        return delim.join(
                (str(self.flashStatIp.total), str(self.flashStatIp.uniq), str(self.flashStatIp.empty),
                str(self.flashStatIpC.total), str(self.flashStatIpC.uniq), str(self.flashStatIpC.empty),
                str(self.flashStatIpB.total), str(self.flashStatIpB.uniq), str(self.flashStatIpB.empty),
                )
               )

class Analyzer(TweakTask):
    NAME = 'spravka_counts'
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'

    NeedPrepare = True
    Weight = 9

    def _MapAll(self, sprMapTableName, flashMapTableName, loginMapTableName):
        self.Trace('Performing map operation...')
        srcTables = utl.MakeTablesList('//logs/yandex-access-log/1d', self.rndRequests.dateStart, self.rndRequests.dateEnd)
        self.Trace('Source tables: %s' % srcTables)

        ipSet = self.rndRequests.GetIpSet()
        dstTables = [flashMapTableName] if not self.moretables else [flashMapTableName, sprMapTableName, loginMapTableName]
        self.RunMap(MapEventSearch(ipSet, self.moretables), srcTables, dstTables)
        for t in dstTables:
            self.RunSort(t, 'key')

        self.Trace('Map done')

    def _ReduceAll(self, sprMapTableName, flashMapTableName, loginMapTableName, resultSprName, resultFlashName, resultLoginName):
        self.Trace('Performing reduce operation...')

        ipSet = self.rndRequests.GetIpSet()
        self.RunReduce(ReduceOp(ipSet), flashMapTableName, resultFlashName, reduce_by='key')
        if self.moretables:
            self.RunReduce(ReduceOp(ipSet), sprMapTableName, resultSprName, reduce_by='key')
            self.RunReduce(ReduceOp(ipSet), loginMapTableName, resultLoginName, reduce_by='key')
        self.Trace('Reduce done')

    def _ReadResults(self, resultSprName, resultFlashName, resultLoginName):
        self.Trace('Reading results..')
        sprResult = {}
        flashResult = {}
        loginResult = {}

        def ReadTable(tableName, resultMap):
            self.Trace('Just before reading from process\'s stdout')
            for r in self.ReadTable(tableName):
                (ip, week) = IpWeekFromKey(r['key'])
                resultMap[r['key']] = Stat(ip = ip, week = week, total = r['total'], uniq = r['uniq'], empty = r['empty'])

            self.Trace('Read table %s, resultMap count = %d' % (tableName, len(resultMap)))

        ReadTable(resultSprName, sprResult)
        ReadTable(resultFlashName, flashResult)
        ReadTable(resultLoginName, loginResult)
        self.Trace('Reading results done')

        return (sprResult, flashResult, loginResult)

    def _Compose(self, sprResult, flashResult, loginResult):
        self.Trace('Start compose')
        for req in self.rndRequests:
            reqidStat = ReqidStat(req.Raw.reqid)

            reqidStat.sprStatIp = sprResult.get(req.WeekKey(), Stat())
            reqidStat.sprStatIpC = sprResult.get(req.WeekKeyClassC(), Stat())
            reqidStat.sprStatIpB = sprResult.get(req.WeekKeyClassB(), Stat())

            reqidStat.flashStatIp = flashResult.get(req.WeekKey(), Stat())
            reqidStat.flashStatIpC = flashResult.get(req.WeekKeyClassC(), Stat())
            reqidStat.flashStatIpB = flashResult.get(req.WeekKeyClassB(), Stat())

            reqidStat.loginStatIp = loginResult.get(req.WeekKey(), Stat())
            reqidStat.loginStatIpC = loginResult.get(req.WeekKeyClassC(), Stat())
            reqidStat.loginStatIpB = loginResult.get(req.WeekKeyClassB(), Stat())

            self.stat[req.Raw.reqid] = reqidStat
        self.Trace('Compose done')

# public:
    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)
        self.rndRequests = rndRequestData
        self.stat = {} # dict: reqid => ReqidStat
        self.moretables = kw.get('moretables')
        self.UpdateBadSubnets()

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_spr_map') as sprMapTable, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_flash_map') as flashMapTable, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_login_map') as loginMapTable, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_spr_red') as resultSpr, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_flash_red') as resultFlash, \
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_login_red') as resultLogin:

            self._MapAll(sprMapTable, flashMapTable, loginMapTable)
            self._ReduceAll(sprMapTable, flashMapTable, loginMapTable,  resultSpr, resultFlash, resultLogin)

            (sprResult, flashResult, loginResult) = self._ReadResults(resultSpr, resultFlash, resultLogin)
            self._Compose(sprResult, flashResult, loginResult)


            self.Trace("End preparing")

        self.SavePrepared(outDir)

    def SavePrepared(self, dirName):
        self.Trace('Saving prepared...')
        fileName = os.path.join(dirName, self.STATE_FILE)
        pickle.dump(self.stat, open(fileName, 'w'))
        self.Trace('Saved prepared')

    def LoadPrepared(self, dirName):
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.stat = pickle.load(open(fileName))

    def UpdateBadSubnets(self):
        self.badSubnets = set()
        for req in self.rndRequests:
            if req.suspicious:
                self.badSubnets.add(ip_utils.StrIpClassC(req.Raw.ip))

    def PrintStat(self):
        for req in self.rndRequests:
            reqidStat = self.stat.get(req.Raw.reqid)
            if reqidStat:
                print reqidStat.ReportToString(req.suspicious, ip_utils.StrIpClassC(req.Raw.ip) in self.badSubnets)

    def SuspiciousInfo(self, reqid):
        #rndReq = self.rndRequests.GetByReqid(reqid)
        #if not rndReq or ipClassC(rndReq.ip) not in self.badSubnets:
        #    return None

        reqidStat = self.stat.get(reqid)
        if not reqidStat:
            return None

        flStat = reqidStat.flashStatIpC
        # TODO add also rule for flStat.unique
        if flStat.total >= 10000 and float(flStat.empty) / flStat.total >= 0.9:
            return SuspInfo(coeff = 1, name = self.NAME, descr = reqidStat.GetShortStatsStr(','))

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

    sprObj = Analyzer(rndReqData)

    if cmd == 'prepare':
        sprObj.Prepare(outDir)
    elif cmd == 'final':
        sprObj.LoadPrepared(outDir)
        sprObj.PrintStat()
    elif cmd == 'check':
        sprObj.LoadPrepared(outDir)
        for i in sys.stdin:
            i = i.strip().split()[0]
            if sprObj.IsSuspicious(i):
                print i


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)

if __name__ == "__main__":
    Main()


