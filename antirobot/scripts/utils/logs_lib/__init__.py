#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os.path
import md5
from datetime import datetime
THIS_DIR = os.path.dirname(sys.argv[0]);
DEPS_PATH = '/place/home/ishfb/bin/'
sys.path = [DEPS_PATH] + sys.path;

from mapreducelib import MapReduce, Record, TemporaryTable

# К сожалению, id-шники событий из antirobot/idl/antirobot.ev приходится хардкодить.
# Гораздо лучше было бы попарсить здесь этот файл и выделить из него названия событий
# и их id-шники, однако мы не можем обращаться к antirobot.ev из этого модуля.
# Это вызвано тем, что модуль копируется на машины MapReduce-кластера, поэтому
# из него нельзя обращаться к другим файлам Аркадии.
EV_ANTIROBOT_FACTORS = 1
EV_CAPTCHA_REDIRECT = 2
EV_CAPTCHA_SHOW = 3
EV_CAPTCHA_CHECK = 4
EV_CAPTCHA_IMAGE = 8
EV_REQUEST_DATA = 6
EV_SERP_COUNTER_TRAINING_REQUEST = 17


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

class DaemonLogRecord:
    def __init__(self, s):
        fields = s.split('\t');
        self.Fields = fields
        self.Severity = fields[0]
        self.MatrixnetResult = fields[2]
        self.HostReqReportInfo = fields[4]
        self.Address = fields[5]
        self.SpravkaAddr = fields[6]
        self.Uid = fields[7]
        self.Addr = fields[8]
        self.StartTime = fields[9]
        self.KarmaInfo = fields[12]
        self.ReqsFromUid = fields[13]
        self.ReqId = fields[14]
        self.RandomCaptcha = bool(fields[15])


def tableExist(tableName):
    return 1 == len(list(MapReduce.getSample(tableName, count = 1)))

def runMrOpCached(op, output):
    if not tableExist(output):
        op()

def runMapCached(mapper, input, output):
    if not tableExist(output):
        MapReduce.runMap(mapper, srcTables = input, dstTable = output)

def runReduceCached(reducer, input, output, sortedOutput = False):
    if not tableExist(output):
        MapReduce.runReduce(reducer, srcTables = input, dstTable = output, sortMode = sortedOutput)

def EventLogTables(dateRange):
    return ["antirobot_eventlog_b/%s" % date for date in dateRange];

def DaemonLogTables(dateRange):
    return ["antirobot_daemon/%s" % date for date in dateRange];

def AccessLogTables(dateRange):
    return ["access_log/%s" % date for date in dateRange];

def TempTableName():
    return 'tmp/' + os.path.basename(sys.argv[0])

class CacheTableGenerator:
    def __init__(self):
        self.md5 = md5.new()
        for arg in sys.argv:
            self.md5.update(arg)
        self.counter = 0

    def gen(self):
        self.counter += 1
        hasher = self.md5.copy()
        hasher.update(str(self.counter))
        return TempTableName() + "_" + hasher.hexdigest()

cacheTableGen = CacheTableGenerator()

MR_SERVER = 'redwood:8013'

MapReduce.useDefaults(verbose = True, usingSubkey = True, server = MR_SERVER, mrExec = 'mapreduce',
                      workDir = ".", scheduleAttrs = {'user' : 'antirobot'});
