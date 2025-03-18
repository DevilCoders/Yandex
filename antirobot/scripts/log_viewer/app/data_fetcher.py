import sys
import os
import datetime
import traceback
from collections import namedtuple
import uuid
import time
import json
import pickle
import subprocess
import itertools

import yt.wrapper as yt
import yt_client
import time_utl
import misc
import yt_cache
import precalc_mgr
from . import cache, app

from errors import ESlowFetchRequired, ELogicError
import random

MAX_ROWS_PORTION = 50

class FetchResult(object):
    def __init__(self, status='unknown'):
        self.status = status
        self.totalRows = None
        self.pageNum = 1
        self.rows = None
        self.descr = None
        self.pendingId = None
        self.reprFields = ('status',)
        self.hint = None

    @staticmethod
    def Unknown(cacheKey):
        res = FetchResult('unknown')
        res.pendingId = str(cacheKey)
        res.reprFields = ('status', 'pendingId', 'descr')
        res.descr = 'Pending operation status is unknown, try to reload the page'
        return res

    @staticmethod
    def Ready(rows, nextRowIndex, atEnd):
        res = FetchResult('ready')
        res.reprFields = ('status', 'rows', 'nextRow', 'atEnd')
        res.rows = rows
        res.nextRow = nextRowIndex
        res.atEnd = atEnd
        return res

    @staticmethod
    def Pending(cacheKey, progress=0, timestamp=None, label=''):
        res = FetchResult('pending')
        res.reprFields = ('status', 'pendingId', 'progress', 'label', 'timestamp')
        res.pendingId = str(cacheKey)
        res.progress = progress
        res.timestamp = timestamp
        res.label = label
        return res

    @staticmethod
    def Error(errorTxt):
        res = FetchResult('error')
        res.reprFields = ('status', 'descr')
        res.descr = errorTxt
        return res

    @staticmethod
    def Finished(pendingId):
        res = FetchResult('finished')
        res.reprFields = ('status', 'pendingId')
        res.pendingId = pendingId
        return res

    @staticmethod
    def Cancel(pendingId, descr=''):
        res = FetchResult('cancel')
        res.reprFields = ('status', 'pendingId', 'descr')
        res.pendingId = pendingId
        res.descr = descr
        return res

    def __str__(self):
        return repr(self.__dict__)

    def Repr(self):
        return dict((x, getattr(self, x)) for x in self.reprFields)


def ChooseKey(ip, yuid):
    if ip:
        return ('ip', ip)
    elif yuid:
        return ('yandexuid', yuid)
    else:
        raise Exception, "IP or yandexuid must be specified"


class BufferedRowsReader:
    def __init__(self, ytInst, defTableName, allRecNumbers, firstRow=0):
        self.ytInst = ytInst
        self.defTableName = defTableName
        self.recNumbers = allRecNumbers[firstRow:]
        self.firstRow = firstRow

    def Next(self):
        if not self.recNumbers:
            return

        i = 0
        while True:
            # first, group by table name
            for tableName, group in itertools.groupby(self.recNumbers, lambda x: x[precalc_mgr.TABLE_NAME_FIELD]):
                rowNumbers = []
                for row in group:
                    rowNumbers.append(str(row[precalc_mgr.ROW_INDEX_FIELD]))
                    i += 1
                    if i == MAX_ROWS_PORTION:
                        break

                numbers = ','.join('#%s' % x for x in rowNumbers)
                tName = '%s[%s]' % (tableName if tableName else self.defTableName, numbers)
                buf = [row for row in self.ytInst.read_table(tName, format=yt.JsonFormat(), raw=False, unordered=True)]

                for rec in buf:
                    yield rec

                if i == len(self.recNumbers) or i == MAX_ROWS_PORTION:
                    return


class DataFetcher(object):
    def __init__(self, precalcMgrCls, clusterRoot, ytInst):
        self.clusterRoot = clusterRoot
        self.ytInst = ytInst
        self.precalcMgr = precalcMgrCls(self.ytInst, self.clusterRoot)

    def IsTableExists(self, table):
        return self.ytInst.exists(table)

    def _GetAdjuster(self, params):
        def DefaultAdjuster(item):
            return item

        return DefaultAdjuster

    def _FindSortedTableToUse(self, dat):
        yesterday = datetime.date.today() - datetime.timedelta(days=1)

        if type(dat) == datetime.datetime:
            dat = dat.date()

        if dat == datetime.date.today():
            result_name = self.precalcMgr.Sorted30minTable()
        else:
            result_name = self.precalcMgr.SortedDayTable(dat)

        if self.IsTableExists(result_name):
            return result_name

        if dat == yesterday:
            result_name = self.precalcMgr.Sorted30minTable(yesterday=True)
            if self.IsTableExists(result_name):
                return result_name

        return None

    # Derived class can define its own transformation
    def _TransformRow(self, row):
        return row

    def _GetRowNumbersFast(self, tableName, ip, yuid, firstRowIndex, **kwargs):
        if not ip and not yuid:
            raise Exception, 'Both ip and yandexuid are empty'

        keyValue = ip if ip else yuid
        return self.precalcMgr.ReadRowNumbersForKey(
                    tableName,
                    keyValue=keyValue
                    )

    def _GetResultRows(self, date, rowNumbers, firstRow=0):
        if len(rowNumbers) == 0 or 'event' in rowNumbers[0]:
            rows = [self._TransformRow({
                'timestamp': row['timestamp'],
                'event': row['event'],
            }) for row in rowNumbers]

            return FetchResult.Ready(
                rows,
                len(rows),
                True  # atEnd
            )


        defTableName = self.precalcMgr.OriginTable(date)
        bufReader = BufferedRowsReader(self.ytInst, defTableName, rowNumbers, firstRow)
        rowFilter = self._GetAdjuster()

        rows = []
        nextRowIndex = firstRow if firstRow != None else 0

        for row in bufReader.Next():
            row = self._TransformRow(row)
            nextRowIndex += 1

            if not rowFilter or rowFilter(row):
                rows.append(row)
                if len(rows) == MAX_ROWS_PORTION:
                    break

        return FetchResult.Ready(
                    rows,
                    nextRowIndex,
                    nextRowIndex >= len(rowNumbers)  # atEnd
                    )


    def _StartSlowSearch(self, date, keyType, keyValue):
        MAX_WAIT = 15

        def Wait(func):
            start = time.time()
            time.sleep(1)
            while not func():
                if time.time() - start > MAX_WAIT:
                    return False
            return True

        try:
            logId = self.precalcMgr.LOG_ID

            cacheKey = cache.MakeKey(logId, date, keyType, keyValue)

            argsPickled = pickle.dumps((logId, date, keyType, keyValue,
                {
                    'yt_proxy': self.ytInst.config['proxy']['url'],
                    'yt_token': self.ytInst.config['token'],
                    'cluster_root': self.clusterRoot,
                }))
            conf = app.config['MY_CONFIG']
            executablePath = os.path.join(conf.APP_BIN_PATH, conf.SLOW_SEARCH_RUNNER)

            if not os.path.exists(executablePath):
                raise Exception('Could not find executable %s' % executablePath)

            subprocess.Popen([executablePath, argsPickled], close_fds=True, preexec_fn=os.setsid)

            if (not Wait(lambda: cache.Exists(cacheKey)) or
                not Wait(lambda: cache.ReadKeyMeta(cacheKey))
                ):
                return FetchResult.Unknown(cacheKey)

            return FetchResult.Pending(cacheKey, progress=None, label='Initializing slow search')
        except:
            return FetchResult.Error(misc.FormatTraceback(traceback.format_exc()))


    @staticmethod
    def GetPendingInfo(pendingId):
        cacheKey = cache.MakeKeyFromPendingId(pendingId)

        if not cache.Exists(cacheKey):
            return FetchResult.Unknown(cacheKey)

        cacheMeta = cache.ReadKeyMeta(cacheKey)
        if not cacheMeta:
            return FetchResult.Unknown(cacheKey)

        if cacheMeta['status'] == 'initializing':
            return FetchResult.Pending(cacheKey, progress=None, label='Initializing YT operation')

        if cacheMeta['status'] == 'running':
            progress = cacheMeta.get('progress')
            value = None
            timestamp = cacheMeta.get('timestamp')
            timestamp = int(timestamp) if timestamp else None

            if progress:
                total = progress.get('total', 100)
                completed = progress.get('completed', 0)
                value = int(float(completed) / total * 100) if total > 0 else 0
            return FetchResult.Pending(cacheKey,
                progress=value,
                timestamp=timestamp,
                label=cacheMeta.get('operation') + ' (%s running jobs)' % progress.get('running'))

        if cacheMeta['status'] == 'ready':
            return FetchResult.Ready(0, 0, False)

        return FetchResult.Error('Something went wrong. %s' % cacheMeta['descr'])

    def _MakeCacheKey(self, date, ip, yuid):
        keyType, keyValue = ChooseKey(ip, yuid)
        return cache.MakeKey(self.precalcMgr.LOG_ID, date, keyType, keyValue)

    def DeleteCache(self, date=None, ip=None, yuid=None):
        cache.DeleteKey(self._MakeCacheKey(date, ip, yuid))

    def GetData(self, date=None, ip=None, yuid=None, slowEnable=False, firstRowIndex=None, updateCache=False, **kwargs):
        cacheKey = self._MakeCacheKey(date, ip, yuid)

        with self.ytInst.Transaction():
            sorted_table_to_use = self._FindSortedTableToUse(date)
            if sorted_table_to_use:
                try:
                    if self.ytInst.is_empty(sorted_table_to_use):
                        raise ELogicError, "Table '%s' is empty. To fix it, rerun precalc process for appropriate table." % str(sorted_table_to_use)

                    rowsNumber = self._GetRowNumbersFast(sorted_table_to_use, ip, yuid, firstRowIndex, **kwargs)
                    return self._GetResultRows(date, rowsNumber, firstRowIndex)
                except ELogicError, ex:
                    return FetchResult.Error(str(ex))

            if cache.Exists(cacheKey) and not updateCache:
                if cache.KeyIsReady(cacheKey):
                    return self.GetResultFromCache(cacheKey, date, ip, yuid, firstRowIndex, **kwargs)

                return FetchResult.Pending(cacheKey)

            originTable = self.precalcMgr.OriginTable(date)
            if not self.IsTableExists(originTable):
                return FetchResult.Error("Table '%s' doesn't exist." % originTable)


        if not slowEnable:
            raise ESlowFetchRequired

        keyType, keyValue = ChooseKey(ip, yuid)
        return self._StartSlowSearch(date, keyType, keyValue)


    def GetResultFromCache(self, cacheKey, date, ip, yuid, firstRowIndex, **kwargs):
        cacheMeta = cache.ReadKeyMeta(cacheKey)
        if not cacheMeta or cacheMeta['status'] == 'error':
            errorText = "<strong>The pending operation has failed.\n" + \
                      "Try to repeat the request. To do that, check the 'Update the cache' checkbox and send the request again.</strong>\n\n" + \
            cacheMeta['descr']

            return FetchResult.Error(misc.FormatTraceback(errorText))

        if cacheMeta['status'] == 'running':
            return FetchResult.Pending(cacheKey, cacheMeta['progress'])

        if cacheMeta['status'] == 'ready':
            return self._GetResultRows(date, self._GetRowNumbersFast(cacheKey.DataPath, ip, yuid, **kwargs), firstRowIndex)

        return self.FetchResult.Error('Unknown long running operation status. Try make the request again with cache update')
