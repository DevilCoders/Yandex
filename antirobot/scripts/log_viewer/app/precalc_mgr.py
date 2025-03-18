import os
import sys
import datetime
import logging
from StringIO import StringIO
import traceback
import json
import time

from yt import wrapper as yt
from yt.wrapper import TablePath
from yt.wrapper.operation_commands import TimeWatcher

import eventlog_adapter
import yt_client
import misc
from errors import ELogicError

from antirobot.scripts.antirobot_eventlog import event
from antirobot.scripts.access_log.request import RequestYTDict, MarketRequestYTDict


MAX_DATE_BACK = 14

ROW_INDEX_FIELD = 'row_index'
TIMESTAMP_FIELD = 'timestamp'
EVENT_FIELD = 'event'
IP_FIELD = 'ip'
YUID_FIELD = 'yandexuid'
TABLE_NAME_FIELD = 'table'
_30MIN_FORMAT = '%Y-%m-%dT%H:%M:%S'
DAY_FORMAT = '%Y-%m-%d'

@yt.with_context
class StdAccesslogMap:
    def __init__(self, keyField, value=None, originTable=None, market=False):
        self.keyField = keyField
        self.value = value
        self.originTable = originTable
        self.market = market

        if keyField == IP_FIELD:
            self.keyFieldGetter = lambda req: req.ip
        elif keyField == YUID_FIELD:
            self.keyFieldGetter = lambda req: req.yandexuid

    def __call__(self, row, context):
        req = RequestYTDict(row) if not self.market else MarketRequestYTDict(row)
        keyValue = self.keyFieldGetter(req)
        if not self.value or self.value == keyValue:
            yield {
                self.keyField: keyValue,
                TIMESTAMP_FIELD: req.time,
                ROW_INDEX_FIELD: context.row_index,
                TABLE_NAME_FIELD: self.originTable
                }


@yt.with_context
class StdEventlogMap:
    def __init__(self, keyField, value=None, originTable=None):
        self.keyField = keyField
        self.value = value
        self.originTable = originTable

        if keyField == IP_FIELD:
            self.fieldGetter = lambda aEv: aEv.Addr()
        elif keyField == YUID_FIELD:
            self.fieldGetter = lambda aEv: aEv.YandexUid()


    def __call__(self, row, context):
        pbEvent = event.Event(str(row['event']))
        if pbEvent.EventType in ('TCacherFactors', 'TAntirobotFactors'):
            return

        aEvent = eventlog_adapter.MakeEventAdapter(pbEvent)

        if not aEvent:
            return

        yield {
            self.keyField: self.fieldGetter(aEvent),
            TIMESTAMP_FIELD: str(aEvent.Timestamp()),
            ROW_INDEX_FIELD: context.row_index,
            TABLE_NAME_FIELD: self.originTable,
            EVENT_FIELD: row['event']
        }


def StdMap(ytInst, mapper, srcTable, dstTable, sync=True):
    return ytInst.run_map(mapper, srcTable, dstTable,
                format=yt.YsonFormat(),
                spec={"job_io": {"control_attributes": {"enable_row_index": True}}, "data_size_per_job": 1024 * 1024 * 1024},
                sync=sync
                )


def StdSortFields(keyField):
    return [keyField, TIMESTAMP_FIELD]


def TableNameFromDate(dat):
    return dat.strftime(DAY_FORMAT)


class FastLogsHelper:
    def __init__(self, precalcMgr, ytInst):
        self.precalcMgr = precalcMgr
        self.ytInst = ytInst

        baseDir = precalcMgr.Abs30minSortedDir()
        yt_client.MkDirSafe(ytInst, baseDir)

        self.resultTable = precalcMgr.Sorted30minTable()

    @staticmethod
    def _GetDateTimeFromTableName(name):
       return datetime.datetime.strptime(name.split('/')[-1], _30MIN_FORMAT)

    @staticmethod
    def _Today():
        today = datetime.date.today()
        return datetime.datetime(today.year, today.month, today.day)

    @staticmethod
    def _FilterByToday(tableNames):
        today = FastLogsHelper._Today()
        return [t for t in tableNames if FastLogsHelper._GetDateTimeFromTableName(t) >= today]

    def _GetAvailableToday30minTables(self):
        return self._FilterByToday(self.ytInst.list(self.precalcMgr.Origin30minDir()))

    def _LastMergedFileName(self):
        return os.path.join(self.precalcMgr.Abs30minSortedDir(), 'last-merged')

    def _ReadLastMergedDateTime(self):
        fileName = self._LastMergedFileName()
        if not self.ytInst.exists(fileName):
            return self._Today()

        text = self.ytInst.read_file(self._LastMergedFileName()).read()
        return self._GetDateTimeFromTableName(text)

    def _WriteLastMergedDateTime(self, dat):
        self.ytInst.write_file(self._LastMergedFileName(), StringIO(dat.strftime(_30MIN_FORMAT)))

    def DropOldTables(self):
        today = self._Today()
        tblToday = TableNameFromDate(today)
        tblYesterday = TableNameFromDate(today - datetime.timedelta(days=1))

        baseDir = self.precalcMgr.Abs30minSortedDir()

        for tname in self.ytInst.list(baseDir):
            if tname.startswith('20') and tname not in (tblToday, tblYesterday):
                self.ytInst.remove(os.path.join(baseDir, tname))

    # ytTransaction param is need to ensure the method called in a context of a transaction
    def Run(self, ytTransaction):
        datetimeLastMerged = self._ReadLastMergedDateTime()
        toPrecalc = []
        for tbl in self._GetAvailableToday30minTables():
            if self._GetDateTimeFromTableName(tbl) > datetimeLastMerged:
                toPrecalc.append(tbl)
        toPrecalc.sort()
        toPrecalc = toPrecalc[:3]

        dstTable = os.path.join(self.precalcMgr.Abs30minSortedDir(), 'new-precalced')
        absOriginPath = lambda x: os.path.join(self.precalcMgr.Origin30minDir(), x)
        for src in toPrecalc:
            StdMap(self.ytInst,
                    self.precalcMgr._GetMapper(originTable=absOriginPath(src), searchFor=None),
                    absOriginPath(src),
                    TablePath(dstTable, append=True, client=self.ytInst),
                    sync=True)

        if not self.ytInst.exists(dstTable):
            return

        self.ytInst.run_sort(dstTable, sort_by=StdSortFields(self.precalcMgr.PRECALC_KEY))
        self._WriteLastMergedDateTime(self._GetDateTimeFromTableName(toPrecalc[-1]))


        if self.ytInst.exists(self.resultTable):
            self.ytInst.run_merge(
                        [self.resultTable, dstTable], self.resultTable,
                        mode='sorted'
                        )
        else:
            self.ytInst.move(dstTable, self.resultTable)

        self.DropOldTables()
        self.ytInst.remove(dstTable, force=True)


class PrecalcMgr(object):
    cached_days = None

    def __init__(self, ytInst, clusterRoot):
        self.ytInst = ytInst
        self.clusterRoot = clusterRoot

    @staticmethod
    def _GetDateFromTableName(name):
        try:
            return datetime.datetime.strptime(name.split('/')[-1], '%Y-%m-%d').date()
        except:
            return None

    def Precalc(self, srcTable, dstTable): # to be overriden
        pass

    def SortedDir(self):
        return self.LOG_ID + '_' + self.PRECALC_KEY

    def AbsSortedDir(self):
        return os.path.join(self.clusterRoot, self.SortedDir())

    def Abs30minSortedDir(self):
        return os.path.join(
            self.clusterRoot,
            'fast',
            self.SortedDir()
            )

    def SortedDayTable(self, day):
        return os.path.join(self.AbsSortedDir(), TableNameFromDate(day))

    def Sorted30minTable(self, yesterday=False):
        dat = datetime.datetime.today()
        if yesterday:
            dat = dat - datetime.timedelta(days=1)

        return os.path.join(self.Abs30minSortedDir(), TableNameFromDate(dat))

    def OriginTable(self, day):
        return os.path.join('//logs', self.LOG_ID, '1d', TableNameFromDate(day))

    def Origin30minDir(self):
        return os.path.join('//logs', self.LOG_ID, '30min')

    def MakeDirIfNeed(self):
        if not self.ytInst.exists(self.AbsSortedDir()):
            self.ytInst.mkdir(self.AbsSortedDir())

    def GetAlreadyPrecalced(self):
        if not self.ytInst.exists(self.AbsSortedDir()):
            return []

        tables = self.ytInst.list(self.AbsSortedDir())

        res = []
        for t in tables:
            if not t.endswith('.lock'):
                dat = self._GetDateFromTableName(t)
                if dat:
                    res.append(dat)

        return res

    def DaysToDeal(self, maxDays):
        if self.cached_days:
            return self.cached_days

        currentDays = set(self.GetAlreadyPrecalced())
        today = datetime.date.today()
        shouldBeDays = set([today - datetime.timedelta(1 + i) for i in xrange(maxDays)])

        actualDays = shouldBeDays.intersection(currentDays)
        toBeDeleted =  currentDays - actualDays
        daysToMake = shouldBeDays - actualDays

        daysToMake = list(daysToMake)
        daysToMake.sort(reverse = True)

        self.cached_days = (daysToMake, toBeDeleted)
        return self.cached_days

    def MissedTables(self, maxDays):
        (missedDays, toBeDeleted) = self.DaysToDeal(maxDays)
        return [self.SortedDayTable(x) for x in missedDays]

    def ReadRowNumbersForKey(self, tableName, keyValue=None):
        """
        Returns items looked like {'row_index': <index>, 'table': <table>}
        All items will be returned when keyValue is none
        """
        exactKey = keyValue if keyValue else None
        tablePath = TablePath(tableName, columns=[ROW_INDEX_FIELD, TABLE_NAME_FIELD, EVENT_FIELD, TIMESTAMP_FIELD], exact_key=exactKey, client=self.ytInst)

        if not self.ytInst.exists(tablePath):
            raise ELogicError, "Table '%s' does not exists" % str(tableName)

        return [x for x in self.ytInst.read_table(tablePath, format=yt.JsonFormat())]

    def _PrecalcTable(self, originTable, resultTable):
        lockName = '%s.lock' % resultTable
        with self.ytInst.Transaction():
            try:
                yt_client.MkDirSafe(self.ytInst, lockName)
                self.ytInst.lock(lockName)

                StdMap(self.ytInst, self._GetMapper(searchFor=None), originTable, resultTable, sync=True)
                self.ytInst.run_sort(resultTable, sort_by=StdSortFields(self.PRECALC_KEY))
                self.ytInst.remove(lockName)
            except:
                logging.info(misc.FormatTraceback(traceback.format_exc()))

    def RunDayly(self, **kw):
        self.MakeDirIfNeed()

        maxDays = kw.get('maxDays', MAX_DATE_BACK)
        daysToMake, toBeDeleted = self.DaysToDeal(maxDays)

        for x in toBeDeleted:
            self.ytInst.remove(self.SortedDayTable(x), force=True)

        for d in daysToMake[:maxDays]:
            originTable = self.OriginTable(d)
            if self.ytInst.exists(originTable):
                self._PrecalcTable(originTable, self.SortedDayTable(d))

    def PrecalcOne(self, day):
        self.MakeDirIfNeed()
        self._PrecalcTable(self.OriginTable(day), self.SortedDayTable(day))


    def SlowSearch(self, cacheKey, srcTable, dstTable, searchFor):
        def UpdateMeta(ytInst, meta):
            jsonStr = json.dumps(meta)
            self.ytInst.write_file(cacheKey.MetaPath, StringIO(jsonStr))

        def WaitOperation(ytInst, op, opName):
            for state in op.get_state_monitor(TimeWatcher(10, 10, 1)):
                progress = op.get_progress()
                UpdateMeta(ytInst, {'status': 'running', 'operation': opName, 'progress': progress, 'timestamp': time.time()})

        ytInst2 = yt_client.CloneInstance(self.ytInst)
        WaitOperation(ytInst2, StdMap(ytInst2, self._GetMapper(searchFor=searchFor), srcTable, dstTable, sync=False), 'Operation 1 of 2: map')
        WaitOperation(ytInst2, ytInst2.run_sort(dstTable, sort_by=StdSortFields(self.PRECALC_KEY), sync= False), 'Operation 2 of 2: sort')

    def PrecalcFastLogs(self):
        try:
            lockName = os.path.join(self.Abs30minSortedDir(), '.lock')
            yt_client.MkDirSafe(self.ytInst, lockName)

            with self.ytInst.Transaction() as tr:
                try:
                    self.ytInst.lock(lockName)
                except:
                    logging.info("Could not get lock - skip precalcing for log %s, key %s" % (self.LOG_ID, self.PRECALC_KEY))
                else:
                    FastLogsHelper(self, self.ytInst).Run(tr)

        except Exception, ex:
            logging.exception(ex)
