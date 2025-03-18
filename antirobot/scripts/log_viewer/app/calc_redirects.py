import os
import imp
import sys
import datetime
import time

from yt import wrapper as yt
from yt.wrapper import TablePath

import log_eventlog

from antirobot.scripts.antirobot_eventlog import event

TargetTable = lambda root: os.path.join(root, 'redirect_counts')
EV_CAPTCHA_REDIRECT = event.EV_TO_ID['TCaptchaRedirect']

def MakeDateStr(dat, withDelim=True):
    delimiter = '-' if withDelim else ''
    return dat.strftime(delimiter.join(['%Y', '%m', '%d']))


def MapOperation(row):
    pbEvent = event.Event(str(row['event']))
    if not pbEvent:
        return

    if pbEvent.EventClassId != EV_CAPTCHA_REDIRECT:
        return

    yield {'value': 1}


def MakeStrRange(datFrom, datTo):
    return '%s - %s' % (MakeDateStr(datFrom, withDelim=True), MakeDateStr(datTo, withDelim=True))


def DoReadTargetTable(ytInst, targetTable):
    for rec in ytInst.read_table(targetTable):
        yield (
            rec['date_range'],
            float(rec['timestamp']) if rec['timestamp'] else 0,
            int(rec['value']) if rec['value'] else 0
            )


def MakeUniqAndReversed(iterable):
    result = list(iterable)
    for tupleIndex in [1, 0]:
        result.sort(key=lambda t: t[tupleIndex], reverse=True)

    used = set()
    return [x for x in result if not (x[0] in used or used.add(x[0]))]


def ReadTargetTable(ytInst, clusterRoot, maxItems=None):
    targetTable = TargetTable(clusterRoot)
    for i, val in enumerate(MakeUniqAndReversed(DoReadTargetTable(ytInst, targetTable))):
        if maxItems == None or i < maxItems:
            yield(val)
        else:
            break

def FindExisting(ytInst, targetTable, dateFrom, dateTo):
    if not ytInst.exists(targetTable):
        return None

    result = None
    key = MakeStrRange(dateFrom, dateTo)
    path = TablePath(targetTable, exact_key=key, client=ytInst)
    for row in ytInst.read_table(path):
        result = int(row['value'])
        break

    return result


def CalcRedirectsCount(ytInst, clusterRoot, endDate, dayCount=1, force=False, verbose=True):
    if not dayCount:
        return

    targetTable = TargetTable(clusterRoot)

    firstDate = endDate - datetime.timedelta(days=dayCount-1)
    if not force and FindExisting(ytInst, targetTable, firstDate, endDate) != None:
        if verbose:
            print >>sys.stderr, "Value for this date period already exists"
        return

    mgr = log_eventlog.PrecalcMgrIp(ytInst, clusterRoot)

    srcTables = []
    for i in xrange(dayCount):
        tName = mgr.OriginTable(firstDate + datetime.timedelta(days=i))
        if ytInst.exists(tName):
            srcTables.append(tName)

    if not srcTables:
        return

    with ytInst.TempTable() as tblNew:
        ytInst.run_map(MapOperation, srcTables, tblNew, spec={"data_size_per_job": 1024 * 1024 * 1024})
        calcResult = ytInst.row_count(tblNew)

    dateRange = MakeStrRange(firstDate, endDate)
    ytInst.write_table(TablePath(targetTable, append=True, client=ytInst),
                        [{
                            'date_range': dateRange,
                            'timestamp': time.time(),
                            'value': calcResult
                        }])
    ytInst.run_sort(targetTable, sort_by=['date_range', 'timestamp'])
