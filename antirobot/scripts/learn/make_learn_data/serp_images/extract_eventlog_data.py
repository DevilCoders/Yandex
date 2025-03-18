# coding: utf-8

import sys
import os.path
from pickle import NONE

from devtools.fleur.util.mapreduce import MapReduce, Record, TemporaryTable

from antirobot.scripts.antirobot_eventlog import event
from antirobot.scripts.antirobot_eventlog.event import Event
from antirobot.scripts.utils import mr_print

import types

EV_ANTIROBOT_FACTORS = event.EV_TO_ID['TAntirobotFactors']
EV_REQUEST_DATA = event.EV_TO_ID['TRequestData']

TRAINING_EVENTS = set([EV_ANTIROBOT_FACTORS]);
REQUEST_DATA_EVENTS = set([EV_REQUEST_DATA]);

class RecordFieldIndices:
    """ Содержит индексы отдельных значений в выходной таблице скрипта.

    Эти индексы соответствуют порядку, в котором значения помещаются в tab-delimited
    список в функциях mapExtractTrainingEvents() и reduceCollectTrainingInfo(). Внося
    изменения в них, не забудьте поправить этот класс.
    """
    REQID_INDEX = 0; # ReqId является ключом выходной таблицы
    IMAGE_DOWNLOADED_INDEX = 1;
    IP_INDEX = 2;
    UID_INDEX = 3;
    XML_SEARCH_INDEX = 4;
    IS_ROBOT_INDEX = 5;
    FACTORS_VERSION_INDEX = 6;
    FACTORS_START_INDEX = 7;

def getUidStr(event):
    return "%s-%s" % (event.Event.Header.UidNs, event.Event.Header.UidId);

def numToIp(ip):
    return "%d.%d.%d.%d" % (ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF)

def date_range(d1, d2):
    from datetime import datetime, timedelta;
    date1 = datetime.strptime(d1, "%Y%m%d");
    date2 = datetime.strptime(d2, "%Y%m%d");
    res = [];
    d = date1;
    while d <= date2:
        res.append(d.strftime("%Y%m%d"));
        d += timedelta(days = 1);
    return res;

def splitListInSmallParts(lst, maxLen):
    return [lst[i : i + maxLen] for i in range(0, len(lst), maxLen)];

def mapExtractTrainingEvents(rec):
    event = Event(rec.value, TRAINING_EVENTS);
    if not event or len(event.Event.Header.Reqid) == 0:
        return
    elif event.EventClassId == EV_ANTIROBOT_FACTORS:
        # Не забудьте исправить класс RecordFieldIndices, внося изменения здесь
        value = '%(ip)s\t%(uid)s\t%(xml)d\t%(isRobot)d\t%(fversion)d\t%(factors)s' % {
             'ip' : event.Event.Header.Addr,
             'uid' : getUidStr(event),
             'xml' : event.Event.InitiallyWasXmlsearch,
             'isRobot' : event.Event.IsRobot,
             'fversion' : event.Event.FactorsVersion,
             'factors' : '\t'.join(['%g' % x for x in event.Event.Factors])};
        yield Record(event.Event.Header.Reqid, "FACTORS", value);


def reduceCollectTrainingInfo(key, recs):
    requestData = None
    imgDownloaded = False
    wasTrainRequest = False

    for rec in recs:
        if rec.subkey == "REQUEST":
            wasTrainRequest = True
        elif rec.subkey == "FACTORS":
            requestData = rec.value
        elif rec.subkey == "DOWNLOAD":
            imgDownloaded = True

    if wasTrainRequest and requestData:
        # Не забудьте исправить класс RecordFieldIndices, внося изменения здесь.
        # requestData - это tab-delimited строка (см. mapExtractTrainingEvents()).
        # Класс RecordFieldIndices задаёт индексы списка, формируемого в следующей строке
        yield Record(key, "RANDOM", '%d\t%s' % (imgDownloaded, requestData))

def mapRequestDataSetKeyReqid(rec):
    event = Event(rec.value, REQUEST_DATA_EVENTS);

    if not event:
        return;

    if len(event.Event.Header.Reqid) == 0:
        return;

    text = event.Event.Data
    if type(text) == types.UnicodeType:
        text = text.replace(u'\t', u' ')
        text = text.encode('utf-8')
    else:
        text = text.replace('\t', ' ')

    text = text.encode('string_escape')

    yield Record(event.Event.Header.Reqid[:100], "REQUEST", "%s\t%s\t%s" % (
        event.Event.Header.Addr, getUidStr(event), text));

def reduceGetRequestDataOfRandomRobots(key, recs):
    isRandom = False;
    reqData = None;
    for rec in recs:
        if rec.subkey == "RANDOM":
            isRandom = True;
        elif rec.subkey == "REQUEST":
            reqData = rec.value;

    if isRandom and reqData is not None:
        yield Record(key, '', reqData.replace('\n', '\\n').replace('\r', '\\r'));


def Execute(mrServer, mrUser, mrProxy, zone, datesRange, mrExec, rndReqDataRaw, rndCaptchas):
    scheduleAttrs = {'net_table': 'mrproxy'}
    if mrUser:
        scheduleAttrs['user'] = mrUser

    allSrcTables = ["antirobot_eventlog_b/%s" % date for date in datesRange];

    MapReduce.useDefaults(verbose=True, usingSubkey=True, server=mrServer,
                          mrExec=mrExec, workDir=".", scheduleAttrs=scheduleAttrs);

    for srcTables in splitListInSmallParts(allSrcTables, 20):
        print >>sys.stderr, "Processing tables: %s" % ", ".join(srcTables);
        with TemporaryTable('tmp/antirobot_features') as tmpTable:
            MapReduce.runMap(mapExtractTrainingEvents, srcTables = srcTables,
                             dstTable = tmpTable.name);
            MapReduce.runReduce(reduceCollectTrainingInfo, srcTable = tmpTable.name,
                                dstTable = tmpTable.name, sortMode = True);

            mr_print.PrintMrTable(tmpTable.name, open(rndCaptchas, 'w'), mrServer, lstrip = True);

            # extract request data
            if rndReqDataRaw:
                print >>sys.stderr, "Extracting request data...";
                MapReduce.runMap(mapRequestDataSetKeyReqid, srcTables = srcTables,
                                 dstTable = tmpTable.name, appendMode = True);
                MapReduce.runReduce(reduceGetRequestDataOfRandomRobots,
                                    srcTable = tmpTable.name, dstTable = tmpTable.name,
                                    sortMode = True);
                with open(rndReqDataRaw, "w") as f:
                    mr_print.PrintMrTable(tmpTable.name, f, mrServer, lstrip=True);
