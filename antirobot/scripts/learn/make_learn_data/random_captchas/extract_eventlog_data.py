#!/usr/bin/env python
# coding: utf-8

import sys
import types
import re

import yt.wrapper as yt
from yt.wrapper import TablePath
import devtools.fleur.util.yt as ytutil

import jsonpickle

from antirobot.scripts.learn.make_learn_data import setup_yt
from antirobot.scripts.learn.make_learn_data import data_types

from antirobot.scripts.antirobot_eventlog import event
from antirobot.scripts.antirobot_eventlog.event import Event


EV_ANTIROBOT_FACTORS = event.EV_TO_ID['TAntirobotFactors']
EV_CAPTCHA_REDIRECT = event.EV_TO_ID['TCaptchaRedirect']
EV_CAPTCHA_SHOW = event.EV_TO_ID['TCaptchaShow']
EV_CAPTCHA_CHECK = event.EV_TO_ID['TCaptchaCheck']
EV_REQUEST_DATA = event.EV_TO_ID['TRequestData']
EV_CAPTCHA_IMAGE_SHOW = event.EV_TO_ID['TCaptchaImageShow']
EV_RANDOM_ROBOT_REMOVE = 11  # deprecated

CAPTCHA_EVENTS = set([EV_CAPTCHA_REDIRECT, EV_CAPTCHA_SHOW, EV_CAPTCHA_CHECK, EV_CAPTCHA_IMAGE_SHOW]);
ANTIROBOT_FACTORS_EVENTS = set([EV_ANTIROBOT_FACTORS]);
RANDOM_ROBOT_REMOVE_EVENTS = set([EV_RANDOM_ROBOT_REMOVE]);
REQUEST_DATA_EVENTS = set([EV_REQUEST_DATA]);
RANDOM_CAPTCHA_COUNT_EVENTS = set([EV_CAPTCHA_REDIRECT, EV_RANDOM_ROBOT_REMOVE]);
ALL_CAPTCHA_EVENTS = set(
    list(CAPTCHA_EVENTS) +
    list(ANTIROBOT_FACTORS_EVENTS) +
    list(RANDOM_ROBOT_REMOVE_EVENTS) +
    list(REQUEST_DATA_EVENTS) +
    list(RANDOM_CAPTCHA_COUNT_EVENTS)
    )

TABLES_BULK_SIZE = 20

WEB_SERVICE_NAME = 'web'


def getUidStr(event):
    return "%s-%s" % (event.Event.Header.UidNs, event.Event.Header.UidId);


def numToIp(ip):
    return "%d.%d.%d.%d" % (ip >> 24, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF)


def findRegexInRequest(reqDataLine, compiledRegex):
    p = compiledRegex.search(reqDataLine.replace('\\n', '\n').replace('\\r', '\r'))
    if p:
        return p.group(1).strip()
    else:
        return None


reHost = re.compile('^host: (.+)$', re.I | re.MULTILINE)
reXHostY = re.compile('^X-Host-Y: (.+)$', re.I | re.MULTILINE)
def extractHost(reqDataLine):
    return findRegexInRequest(reqDataLine, reXHostY) or findRegexInRequest(reqDataLine, reHost)


reService = re.compile('^X-Antirobot-Service-Y: (.+)$', re.I | re.MULTILINE)
def extractService(reqDataLine):
    return findRegexInRequest(reqDataLine, reService) or WEB_SERVICE_NAME


def getTLD(host):
    host = host.split(':',1)[0]
    return host.split('.')[-1]


def makeZoneFilter(zone):
    COM_TLDS = ('com',)
    CUBR_TLDS = ('ru', 'kz', 'ua', 'by')
    TR_TLDS = ('tr', )

    ZONE_TO_TLDS = {
        'tr': TR_TLDS,
        'cubr': CUBR_TLDS,
        'com': COM_TLDS,
        'cubr_com': COM_TLDS + CUBR_TLDS
    }

    def filterByTLD(reqData, tlds):
        "returns True if request has a host with specified tld's"
        host = extractHost(reqData)
        if not host:
            return True

        if getTLD(host) in tlds:
            return True

        return False

    if zone == 'all':
        return lambda req: True

    tlds = ZONE_TO_TLDS.get(zone)
    if not tlds:
        raise Exception, "Bad zone value"

    return lambda req: filterByTLD(req, tlds)


def makeServiceFilter(serviceList):
    if not serviceList:
        return lambda req: True

    return lambda serviceName: serviceName in serviceList



def IsIpUid(uidStr):
    return uidStr.startswith("1-")


class MapRequestDataSetKeyReqid:
    def __init__(self, zone, serviceList):
        self.zone = zone
        self.zoneFilter = None
        self.serviceList = serviceList or []
        self.serviceFilter = None

    def __call__(self, rec):
        # placed here due python pickle problems
        if not self.zoneFilter:
            self.zoneFilter = makeZoneFilter(self.zone)

        if not self.serviceFilter:
            self.serviceFilter = makeServiceFilter(self.serviceList)

        event = Event(str(rec['event']), REQUEST_DATA_EVENTS)

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
        service = extractService(text)
        if self.zoneFilter(text) and self.serviceFilter(service):
            data = data_types.RndReqDataRaw()
            data.reqid = event.Event.Header.Reqid[:100]
            data.ip = event.Event.Header.Addr
            data.uidStr = getUidStr(event)
            data.service = service
            data.request = text

            yield {'reqid': data.reqid, 'tag': "REQUEST", 'data': data.ToString()}


def mapSetKeyTokenFilterRandom(rec):
    event = Event(str(rec['event']), CAPTCHA_EVENTS)

    if not event:
        return

    token = event.Event.Token;
    if token.startswith('1') or token.startswith('2'):
        yield {'token': token, 'timestamp': str(event.Timestamp), 'event': rec['event']}


def reduceExtractGoodRandomCaptchaReqids(key, recs):
    reqids = []
    data = data_types.CaptchaData()

    for rec in recs:
        event = Event(rec['event'])
        evClass = event.EventClassId

        if evClass == EV_CAPTCHA_REDIRECT:
            data.Ident.uidStr = getUidStr(event)
            data.Ident.ip = event.Event.Header.Addr
            reqids.append((event.Event.Header.Reqid, event.Timestamp))
        elif evClass == EV_CAPTCHA_SHOW:
            data.Flags.wasShow = True;
        elif evClass == EV_CAPTCHA_IMAGE_SHOW:
            data.Flags.wasImageShow = True
        elif evClass == EV_CAPTCHA_CHECK:
            data.Flags.wasAttempt = True
            data.Flags.wasSuccess = bool(event.Event.Success)
            data.Flags.wasEnteredHiddenImage = bool(event.Event.EnteredHiddenImage)

    for (reqid, ts) in reqids:
        if len(reqid) > 0:
            data.Ident.ts = ts
            yield {'reqid': reqid, 'tag': 'CAPTCHA', 'data': data.ToString()}


def reduceByZonedRequestData(key, recs):
    result = []
    hasReqData = False
    for rec in recs:
        if rec['tag'] == 'REQUEST':
            hasReqData = True
        else:
            result.append(rec)

    if hasReqData:
        for rec in result:
            #yield {'reqid': rec['reqid'], 'tag': rec['tag'], 'data': rec['data']}
            yield rec


def mapAntirobotFactorsSetKeyReqid(rec):
    event = Event(str(rec['event']), ANTIROBOT_FACTORS_EVENTS)

    if not event:
        return;

    if len(event.Event.Header.Reqid) == 0:
        return;

    yield {'reqid': event.Event.Header.Reqid[:100],'tag': 'FACTORS', 'data': rec['event']}


def reduceJoinCaptchaDataAndFactors(key, recs):
    captchaData = None
    event = None

    for rec in recs:
        if rec['tag'] == "CAPTCHA":
            captchaData = data_types.CaptchaData.FromString(rec['data'])
        elif rec['tag'] == "FACTORS":
            event = Event(rec['data']);

    if captchaData:
        captchaAndFactors = data_types.CaptchaDataAndFactors()
        if event:
            uid = getUidStr(event)
            captchaAndFactors.Ident = captchaData.Ident.Copy()
            captchaAndFactors.Ident.reqid = key['reqid']
            captchaAndFactors.Ident.ip = event.Event.Header.Addr
            captchaAndFactors.Ident.uidStr = uid
            captchaAndFactors.Flags = captchaData.Flags.Copy()
            captchaAndFactors.Flags.wasXmlSearch = bool(event.Event.InitiallyWasXmlsearch)
            captchaAndFactors.Flags.isRobot = bool(event.Event.IsRobot)
            captchaAndFactors.Factors.version = event.Event.FactorsVersion
            captchaAndFactors.Factors.AssignFactors(event.Event.Factors)
            if not captchaAndFactors.Factors.IsEmpty():
                yield {'uid': uid, 'timestamp': str(captchaData.Ident.ts), 'data': captchaAndFactors.ToString()}
        else:
            captchaAndFactors.AssignCaptchaData(captchaData)
            captchaAndFactors.Ident.reqid = key['reqid']

            yield {'uid': captchaData.Ident.uidStr, 'timestamp': str(captchaData.Ident.ts), 'tag': 'captcha_and_factors', 'data': captchaAndFactors.ToString()}


def mapRandomRobotRemoveSetKeyUid(rec):
    event = Event(str(rec['event']), RANDOM_ROBOT_REMOVE_EVENTS)

    if not event:
        return;

    yield {'uid': getUidStr(event), 'timestamp': str(event.Timestamp), 'tag': 'RANDOM_ROBOT_REMOVE'}


def mapCountRandomCaptchaRedirectsOfUid(rec):
    event = Event(str(rec['event']), RANDOM_CAPTCHA_COUNT_EVENTS)

    if not event:
        return;

    evClass = event.EventClassId;

    if evClass == EV_CAPTCHA_REDIRECT:
        token = event.Event.Token;
        if token.startswith('1'):
            yield {'uid': getUidStr(event), 'timestamp': str(event.Timestamp), 'tag': 'RANDOM_CAPTCHA_REDIRECT'}
    elif evClass == EV_RANDOM_ROBOT_REMOVE:
        yield {'uid': getUidStr(event), 'timestamp': str(event.Timestamp), 'tag': 'RANDOM_ROBOT_REMOVE'}


def reduceCountRandomCaptchaRedirectsOfUid(key, recs):
    data = data_types.NumRedirData()

    for rec in recs:
        if rec['tag'] == "RANDOM_CAPTCHA_REDIRECT":
            data.numRedirs += 1;
        elif rec['tag'] == "RANDOM_ROBOT_REMOVE":
            data.numRemovals += 1;
    yield {'uid': key['uid'], 'tag': "0000 NUM_REDIRECTS", 'data': data.ToString()}


def reduceExtractCorrectReqsFromUid(key, recs):
    MAX_RECORDS = 10

    def makeRecord(rec, numRedirsData, flags=None):
        captchaAndFactors = data_types.CaptchaDataAndFactors.FromString(rec['data'])
        #captchaAndFactors.FromString(rec.value)

        finalData = data_types.FinalCaptchaData()
        finalData.AssignCaptchaDataAndFactors(captchaAndFactors)
        finalData.AssignNumRedirs(numRedirsData)

        if flags:
            finalData.AssignFlags(flags)

        return {'reqid': finalData.Ident.reqid, 'tag': 'RANDOM', 'data': finalData.ToString()}

    def isRobotRemove(rec):
        return "RANDOM_ROBOT_REMOVE" == rec.get('tag')

    def GetFlagsFromRec(rec):
        data = data_types.CaptchaDataAndFactors.FromString(rec['data'])
        #data.FromString(rec.value)

        return data.Flags.Copy()

    def processIpUid(recs):
        numRedirData = data_types.NumRedirData()
        prevRec = None
        for curRec in recs:
            if curRec.get('tag') == "0000 NUM_REDIRECTS":
                numRedirData = data_types.NumRedirData.FromString(curRec['data'])
                continue

            if prevRec:
                # We don't yield the previous record only if it doesn't contain IMAGE_SHOW flag
                # and the current one is RANDOM_ROBOT_REMOVE
                prevFlags = GetFlagsFromRec(prevRec)
                if not isRobotRemove(curRec) or prevFlags.wasImageShow:
                    yield makeRecord(prevRec, numRedirData)

                prevRec = None

            if not isRobotRemove(curRec):
                data = data_types.CaptchaDataAndFactors.FromString(curRec['data'])
                #data.FromString(curRec.value)
                if not data.Factors.IsEmpty():
                    prevRec = curRec

        if prevRec:
            yield makeRecord(prevRec, numRedirData)

    def processNonIpUid(recs):
        combinedFlags = data_types.Flags()
        firstRecord = None
        numRedirsData = data_types.NumRedirData()
        numRecords = 0

        for rec in recs:
            if rec.get('tag') == "0000 NUM_REDIRECTS":
                numRedirsData = data_types.NumRedirData.FromString(rec['data'])
                continue;

            if isRobotRemove(rec):
                if firstRecord and combinedFlags.wasImageShow:
                    yield makeRecord(firstRecord, numRedirsData, flags=combinedFlags);

                combinedFlags = data_types.Flags()
                firstRecord = None
                numRecords = 0
            else:
                data = data_types.CaptchaDataAndFactors.FromString(rec['data'])
                #data.FromString(rec.value)

                if not firstRecord and not data.Factors.IsEmpty():
                    firstRecord = rec

                if numRecords < MAX_RECORDS:
                    flags = GetFlagsFromRec(rec)
                    combinedFlags.CombineWith(flags)

                numRecords += 1;

        if firstRecord:
            yield makeRecord(firstRecord, numRedirsData, flags=combinedFlags)

    if IsIpUid(key['uid']):
        return processIpUid(recs)
    else:
        return processNonIpUid(recs)


def reduceGetRequestDataOfRandomRobots(key, recs):
    isRandom = False;
    reqData = None;
    for rec in recs:
        if rec['tag'] == "RANDOM":
            isRandom = True;
        elif rec['tag'] == "REQUEST":
            reqData = rec['data']

    if isRandom and reqData is not None:
        yield {'reqid': key['reqid'], 'data': reqData}


def splitListInSmallParts(lst, maxLen):
    return [lst[i : i + maxLen] for i in range(0, len(lst), maxLen)];


def splittedMap(mapFunc, allSrcTables, dstTable):
    for srcTables in splitListInSmallParts(allSrcTables, TABLES_BULK_SIZE):
        print >>sys.stderr, "Processing tables: %s" % ", ".join(srcTables);
        yt.run_map(mapFunc, srcTables, dstTable)


def TouchFile(fileName):
    open(fileName, 'w').close()


def GetTableName(datStr):
    return '//logs/antirobot-binary-event-log/1d/%s' % datStr


def SaveRndCaptchas(tblName, rndCaptchas):
    hFile = open(rndCaptchas, 'w')

   # data = data_types.FinalCaptchaData()
    for rec in yt.read_table(tblName):
        data = data_types.FinalCaptchaData.FromString(rec['data'])
        print >>hFile, data.ToString()


def SaveRndReqDataRaw(tblName, rndReqDataRaw):
    hFile = open(rndReqDataRaw, 'w')

    for rec in yt.read_table(tblName):
        dataStr = rec['data']

        # check the data is valid
        data = data_types.RndReqDataRaw.FromString(rec['data'])

        print >>hFile, dataStr


def Execute(ytProxy, ytToken, zone, serviceList, datesRange, rndReqDataRaw, rndCaptchas):
    with ytutil.ModulesArchiveWriter() as archive_writer:
        setup_yt.SetupYT(ytProxy, ytToken, archive_writer)

        TouchFile(rndReqDataRaw)
        TouchFile(rndCaptchas)

        allSrcTables = [GetTableName(date) for date in datesRange]
        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_request_data.') as tmpReqDataTable,\
             yt.TempTable('//home/antirobot/tmp', prefix='antirobot_features') as tmpFeatTable:

            # Извлекаем события RequestData, фильтруем по зоне
            splittedMap(MapRequestDataSetKeyReqid(zone, serviceList), allSrcTables,
                            TablePath(tmpReqDataTable, append=True))

            # Собираем в одну таблицу все события, связанные с капчей, если токен начинается на '1' или '2' (случайная капча)
            splittedMap(mapSetKeyTokenFilterRandom, allSrcTables,
                            TablePath(tmpFeatTable, append=True))

            yt.run_sort(tmpFeatTable, sort_by=['token', 'timestamp'])
            # Собираем CaptchaData
            yt.run_reduce(reduceExtractGoodRandomCaptchaReqids, tmpFeatTable, tmpFeatTable, reduce_by='token')

            # Добавляем факторы
            splittedMap(mapAntirobotFactorsSetKeyReqid, allSrcTables, TablePath(tmpFeatTable, append=True))
            yt.run_sort(tmpFeatTable, sort_by='reqid')

            # Добавляем RequestData в текущую таблицу
            yt.run_merge([tmpReqDataTable, tmpFeatTable], tmpFeatTable)
            yt.run_sort(tmpFeatTable, sort_by='reqid')

            # Фильтруем таблицу по наличию RequestData из требуемой зоны
            yt.run_reduce(reduceByZonedRequestData, tmpFeatTable, TablePath(tmpFeatTable, sorted_by=['reqid']), reduce_by='reqid')

            # Объединяем CaptchaData, факторы
            yt.run_reduce(reduceJoinCaptchaDataAndFactors, tmpFeatTable, tmpFeatTable, reduce_by='reqid')

            # Извлекаем записи RANDOM_ROBOT_REMOVE
            splittedMap(mapRandomRobotRemoveSetKeyUid, allSrcTables, TablePath(tmpFeatTable, append=True))

            with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_features_counts.') as tmpTableForCount:
                # Извлекаем события редиректов и random_robot_remove
                splittedMap(mapCountRandomCaptchaRedirectsOfUid, allSrcTables, TablePath(tmpTableForCount, append=True))

                yt.run_sort(tmpTableForCount, sort_by=['uid', 'timestamp'])
                # Считаем редиректы и события RANDOM_ROBOT_REMOVE для uid, и добавляем их в таблицу (3)
                yt.run_reduce(reduceCountRandomCaptchaRedirectsOfUid, tmpTableForCount,
                                TablePath(tmpFeatTable, append=True), reduce_by='uid')

            # Формируем финальную таблицу с факторами и данными по капче
            yt.run_sort(tmpFeatTable, sort_by=['uid', 'timestamp'])
            yt.run_reduce(reduceExtractCorrectReqsFromUid, tmpFeatTable, tmpFeatTable, reduce_by='uid')
            yt.run_sort(tmpFeatTable, sort_by='reqid')

            # Формируем таблицу с RequestData
            yt.run_merge([tmpFeatTable, tmpReqDataTable], tmpReqDataTable)
            yt.run_sort(tmpReqDataTable, sort_by='reqid')
            yt.run_reduce(reduceGetRequestDataOfRandomRobots, tmpReqDataTable, tmpReqDataTable, reduce_by='reqid')
            yt.run_sort(tmpReqDataTable, sort_by='reqid')

            # Сохраняем полученные результаты
            SaveRndCaptchas(tmpFeatTable, rndCaptchas)
            SaveRndReqDataRaw(tmpReqDataTable, rndReqDataRaw)


#
# Как подготовить выборку для тестов
#
# 1. Каким-либо способом подготовить список уидов, запросы от которых составят выборку для тестов
#    (Например, можно запустить выборку за один день а затем вручную выбрать 10-20 уидов из этой выборки)
#
# 2. Заполнить этими уидами поле TestDataMap.Uids
#
# 3. Выполнить ./make_learn_data extract_test_data DATE > requests
#
# 4. Отредактировать файл requests с запросами, выкинуть лишние события (для уменьшения выборки и ускорения работы тестов)
#    (в этом файле запросы отсортированы по уиду, затем по таймстампу)
#
# TODO:
# 6. Проверка выборки:
#    Загрузить сконвертированный файл в локальный mapreduce ./mapreduce -server local -lenval -write antirobot_eventlog_b/DATE < converted_requests
#    Запустить сборку выборки в этом локальном мапредьюсе:
#    ./make_learn_data -s local --work <path> make DATE
#    Проверить, что собранная выборка тождественна той выборке, что была загружена в локальный мапредьюс
#
# 7. Если выборка в порядке, поправить тесты
#
# 8. Далее, загрузить выборку в сандбокс
#    Для этого:
#       $ytest.py get-data -o antirobot.scripts.learn.test_data -t input -p ./test_data
#       $cp converted_requests ./test_data/data
#       $ytest.py put-data -o antirobot.scripts.learn.test_data -t input -p ./test_data

class TestDataMap:
    def __init__(self):
        # Theese uids were extracted from antirobot-binary-event-log/1d/20170131
        self.Uids = (
            ### ROBOTS
            # autoru
            #'2-1533047495754284796',
            # avia
            #'1-1533653278',
            # kinopoisk
            #'2-1533005695506025074',
            # market
            #'8-4060319422210640682',
            # taxi
            #'1-922116041',
            # web
            '6-1263526929875173061',
            #'8-10265193871948644640',
            # webmaster
            #'7-268915142',
            # NOT ROBOTS
            # autoru
            #'1-1319030988',
            #'2-1532517639135277701',
            #'6-1002472284910389448',
            # avia
           # '1-1604554777',
            #'6-7509330184887809850',
            #'7-1130000001560438',
            # kinopisk
            #'10-1005877481520262428',
            #'1-3585141898',
            #'6-5086362970751078534',
            # market
            #'10-1009156151531770462',
            #'1-1844631322',
            #'7-100235918',
            # rabota
            #'1-1583774262',
            #'6-3060390145277582230',
            #'7-109549719',
            # realty
            #'2-1532718465739884882',
            #'6-1214456305746063221',
           # '7-155986947',
            # taxi
           # '7-503755222',
           # '6-4897833281105983092',
           # '1-3579285919',
            #tech
           # '6-1229803168779778936',
            # web
           # '10-1001152051532022956',
           # '1-100304661',
           # '2-1532501134133100172',
           # '6-1000000333623349687',
           # '7-100008490',
           # '8-10000435818487087392',
            # webmaster
           # '1-1841962198',
           # '7-8939059',
            # wordstat
          #  '7-246342889',

            # COM zone
          #  '1-2972043499',
          #  '1-633422076',
          #  '1-2783177006',
          #  '10-7596100821532619912',
        )

    def __call__(self, rec):
        event = Event(str(rec['event']), ALL_CAPTCHA_EVENTS)
        if not event:
            return

        uidStr = getUidStr(event)
        if uidStr in self.Uids:
            yield {'uid': uidStr, 'timestamp': str(event.Timestamp), 'event_type': event.EventType, 'event': rec['event']}


def ExtractTestData(ytProxy, ytToken, datesRange):
    with ytutil.ModulesArchiveWriter() as archive_writer:
        setup_yt.SetupYT(ytProxy, ytToken, archive_writer)

        allSrcTables = [GetTableName(date) for date in datesRange]
        with yt.TempTable('//home/antirobot/tmp', prefix='antirobot_extract_result') as tmpResult:
            yt.run_map(TestDataMap(), allSrcTables, tmpResult)
            yt.run_sort(tmpResult, sort_by='uid')

            for row in yt.read_table(tmpResult, format='json'):
                print jsonpickle.dumps(row)
