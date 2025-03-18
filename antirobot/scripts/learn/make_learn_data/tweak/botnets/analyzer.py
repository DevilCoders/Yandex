import os
import random
import operator
from operator import attrgetter
import pickle
import datetime
from itertools import imap
from collections import defaultdict, namedtuple

from id2time_ip import id2time_ip, ShortenKey
from count_max_ips import count_max_ips, IpCounter

from antirobot.scripts.learn.make_learn_data.tweak.tweak_task import TweakTask
from antirobot.scripts.learn.make_learn_data.tweak.susp_info import SuspInfo


DEF_THREAD_NUM = 1

class RightShifter(object):
    def __init__(self, bits_num):
        self.bits_num = bits_num
    def __call__(self, ip):
        return ip >> self.bits_num

subnet24 = RightShifter(8)
subnet16 = RightShifter(16)

secs_per_hour = 60 * 60
secs_per_day = secs_per_hour * 24

IpCounters = map(IpCounter, *zip(
        ('day',     None,       secs_per_day,   10),
        ('hour',    None,       secs_per_hour,  10),
        ('day24',   subnet24,   secs_per_day,   10),
        ('hour24',  subnet24,   secs_per_hour,  10),
        ('day16',   subnet16,   secs_per_day,   10),
        ('hour16',  subnet16,   secs_per_hour,  10),
) )

Cookies = ('spravka', 'fuid01', 'L', 'yandex_login')
#Cookies = ('yandex_login',)

class Stat(object):
    __slots__ = ["day", "hour", "day24", "hour24", "day16", "hour16"]

    def __init__(self, **kw):
        self.day = kw.get('day',0 )
        self.hour = kw.get('hour', 0)
        self.day24 = kw.get('day24',0 )
        self.hour24 = kw.get('hour24', 0)
        self.day16 = kw.get('day16',0 )
        self.hour16 = kw.get('hour24', 0)

    def __str__(self):
        return self.GetStr('\t')

    def GetStr(self, delim):
        return delim.join(map(str, (self.day, self.hour, self.day24, self.hour24, self.day16, self.hour16)))

    def __getstate__(self):
        res  = {}
        for attr in self.__slots__:
            res[attr] = getattr(self, attr)
        return res

    def __setstate__(self, state):
        for attr in self.__slots__:
            setattr(self, attr, state[attr])

class AllStat(object):
    __slots__ = ["reqid", "spravkaStat", "fuidStat", "lStat", "yandexLoginStat"]

    def __init__(self, reqid = None):
        self.reqid = reqid
        self.spravkaStat = Stat()
        self.fuidStat = Stat()
        self.lStat = Stat()
        self.yandexLoginStat = Stat()

    def __str__(self):
        return '\t'.join(map(str, (self.reqid, self.spravkaStat, self.fuidStat, self.lStat, self.yandexLoginStat)))

    def GetStr(self, delim = ','):
        return delim.join(map(lambda x: x.GetStr(','), (self.spravkaStat, self.fuidStat, self.lStat, self.yandexLoginStat)))

    def GetFullReport(self, reqidUids):
        values = (self.reqid,
                  str(self.spravkaStat),
                  str(self.fuidStat),
                  str(self.lStat),
                  str(self.yandexLoginStat),
                  reqidUids.spravka, reqidUids.fuid, reqidUids.L, reqidUids.yandex_login
                 )

        values = [str(x) if x else '<EMPTY>' for x in values]

        return '\t'.join(values)

    def __getstate__(self):
        res  = {}
        for attr in self.__slots__:
            res[attr] = getattr(self, attr)
        return res

    def __setstate__(self, state):
        for attr in self.__slots__:
            setattr(self, attr, state[attr])

UidsReqid = namedtuple('UidsReqid', ('spravka', 'fuid', 'L', 'yandex_login'))

class Analyzer(TweakTask):
    SIMULATE_COUNT = 100000
    STATE_FILE = 'prepared.state'
    NAME = 'botnets'

    NeedPrepare = True
    Weight = 8

    def __init__(self, rndRequestData, **kw):
        TweakTask.__init__(self)
        self.rndRequests = rndRequestData
        self.reqidUids = defaultdict(lambda: UidsReqid(None, None, None, None))
        self.uidReqids = {} # uid => reqid (uid is spravka, fuid, L, yandex_login)
        self.badReqids = {} # reqid => AllStat

    def _ReadResults(self, tmpDir):
        self.Trace('Just before reading from process\'s stdout')
        def ReadBadReqidStats(table, statGetter):
            for r in self.ReadTable(table):
                reqid = self.uidReqids.get(r['key'])
                if reqid:
                    allstat = self.badReqids.get(reqid)
                    if not allstat:
                        allstat = AllStat(reqid)
                        self.badReqids[reqid] = allstat

                    valueList = r['value']
                    st = statGetter(allstat)
                    (st.day, st.hour, st.day24, st.hour24, st.day16, st.hour16) = valueList[0:6]

        ReadBadReqidStats(os.path.join(tmpDir, 'spravka'), operator.attrgetter('spravkaStat'))
        ReadBadReqidStats(os.path.join(tmpDir, 'fuid01'), operator.attrgetter('fuidStat'))
        ReadBadReqidStats(os.path.join(tmpDir, 'L'), operator.attrgetter('lStat'))
        ReadBadReqidStats(os.path.join(tmpDir, 'yandex_login'), operator.attrgetter('yandexLoginStat'))

        self.Trace('End of reading')


    def UpdateRndReqFull(self, rndReqFullIter):
        def Get(cookieName, cookie):
            if cookie:
                return ShortenKey(cookieName, cookie.value)
            else:
                return ''

        for req in rndReqFullIter:
            reqid = req.Raw.reqid

            spravka = req.cookies.get('spravka')
            if spravka:
                self.uidReqids[Get('spravka', spravka)] = reqid

            fuid = req.cookies.get('fuid01')
            if fuid:
                self.uidReqids[Get('fuid01', fuid)] = reqid

            L = req.cookies.get('L')
            if L:
                self.uidReqids[Get('L', L)] = reqid

            yandex_login = req.cookies.get('yandex_login')
            if yandex_login:
                self.uidReqids[Get('yandex_login', yandex_login)] = reqid

            self.reqidUids[reqid] = UidsReqid(spravka = Get('spravka', spravka),
                                                fuid =Get('fuid01', fuid),
                                                L = Get('L', L),
                                                yandex_login =Get('yandex_login', yandex_login))

    def Prepare(self, outDir, always=False):
        self.Trace("Start preparing")
        if not always and os.path.exists(os.path.join(outDir, self.STATE_FILE)):
            self.Trace("Already prepared - exiting")
            return

        max_window = max(imap(attrgetter('time_window'), IpCounters))
        more_days = (max_window + secs_per_day - 1) // secs_per_day

        dateBegin = self.rndRequests.dateStart
        dateEnd = self.rndRequests.dateEnd

        dateBegin -= datetime.timedelta(more_days)
        dateEnd += datetime.timedelta(more_days)

        tmpDir = '//home/antirobot/tmp/antirobot_%08x' % random.randrange(0xffffffff)
        dstPrefix = '//home/antirobot/tmp/antirobot_dst_%08x' % random.randrange(0xffffffff)

        self.YtMkDir(tmpDir)
        self.YtMkDir(dstPrefix)
        try:
            id2time_ip(self, '//logs/yandex-access-log/1d', tmpDir, Cookies, dateBegin, dateEnd)

            self.Trace("Start reduce operation")
            threadCount = 1 # if self.mrInstance.default_options.Server == 'local' else 2
            count_max_ips(self, tmpDir, dstPrefix, Cookies, IpCounters, threadCount)
            self._ReadResults(dstPrefix)

            self.Trace("End preparing")
        finally:
            self.YtDropDir(tmpDir)
            self.YtDropDir(dstPrefix)

        self.SavePrepared(outDir)


    def LoadPrepared(self, dirName):
        self.Trace('Start loading prepared data from %s' % dirName)
        fileName = os.path.join(dirName, self.STATE_FILE)
        if os.path.exists(fileName):
            self.badReqids = pickle.load(open(fileName))
        self.Trace("Prepared data loaded, len: %s" % len(self.badReqids))

    def SavePrepared(self, dirName):
        self.Trace('Saving prepared...')
        fileName = os.path.join(dirName, self.STATE_FILE)
        pickle.dump(self.badReqids, open(fileName, 'w'))

    def PrintStat(self):
        for reqid, stat in self.badReqids.iteritems():
            rndReq = self.rndRequests.GetByReqid(reqid)
            suspFlag = rndReq.suspicious if rndReq else 0
            suspFlag = '1' if suspFlag else '0'
            print '\t'.join((suspFlag, stat.GetFullReport(self.reqidUids[reqid])))

    def SuspiciousInfo(self, reqid):
        allStat = self.badReqids.get(reqid)
        if not allStat:
            return None

        info = []
        for statField in ('spravkaStat', 'fuidStat', 'lStat', 'yandexLoginStat'):
            stat = getattr(allStat, statField)
            if int(stat.hour16) > 10:
                info.append('%s:%s' % (statField, stat.GetStr(',')))

        if info:
            return SuspInfo(coeff = 1, name = self.NAME, descr = '/'.join(info))
        else:
            return None


from antirobot.scripts.learn.make_learn_data.tweak.tweak_list import Register
Register(Analyzer)
