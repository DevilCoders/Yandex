#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import argparse
import datetime
import time
from datetime import timedelta

THIS_DIR = os.path.dirname( os.path.realpath(__file__))

sys.path = [THIS_DIR + '/../'] + sys.path  # for logs_lib and mr_print
sys.path = [THIS_DIR + '/../..'] + sys.path # for antirobot_eventlog

from logs_lib import *
import mr_print
import antirobot_eventlog

DESCRIPTION = """Extracts TCaptchaImageShow, TCaptchaCheck, TCaptchaShow, TCaptchaRedirect events
from event log for the given time period and converts them Logstalgia log format.

Don't use _captcha_events.py, use captcha_events.sh instead.

You can find Logstalgia at https://code.google.com/p/logstalgia/ and its git repository https://github.com/acaudwell/Logstalgia."""

EVENT_TO_REQUEST = {EV_CAPTCHA_REDIRECT : 'redirect', 
                    EV_CAPTCHA_SHOW : '/showcaptcha', 
                    EV_CAPTCHA_CHECK : '/checkcaptcha', 
                    EV_CAPTCHA_IMAGE : '/captchaimg'}
                    
EVENT_TO_RESPONSE = {EV_CAPTCHA_REDIRECT : '302', 
                     EV_CAPTCHA_SHOW : '200', 
                     EV_CAPTCHA_IMAGE : '200'}
                     
RESPONSE_TO_COLOR = {'200' : '00FF00', '302' : 'FFFF00', 'OK' : '007F00'}

def parseDateTime(value):
    try:
        return datetime.strptime(value, "%Y%m%d-%H:%M:%S")
    except ValueError:
        return datetime.strptime(value, "%Y%m%d")

def dateRange(date1, date2):
    from datetime import datetime, timedelta
    res = [];
    d = date1;
    while d <= date2:
        res.append(d.strftime("%Y%m%d"))
        d += timedelta(days = 1)
    return res;
    
def datetime2timestamp(dt):
    return int(time.mktime(dt.timetuple()))

def reqid2timestamp(reqid):
    return reqid.split('-')[0][:-6]

class CaptchaEventsMapper:
    def __init__(self, startTimestamp, endTimestamp, correctCaptchaSuccess, wrongCaptchaSuccess):
        self.startTimestamp = startTimestamp
        self.endTimestamp = endTimestamp
        self.correctCaptchaSuccess = correctCaptchaSuccess
        self.wrongCaptchaSuccess = wrongCaptchaSuccess
        
    def __call__(self, rec):
        event = antirobot_eventlog.Event(rec.value, [EV_CAPTCHA_REDIRECT, EV_CAPTCHA_SHOW, EV_CAPTCHA_CHECK, EV_CAPTCHA_IMAGE])
        if not event:
            return
        try:
            timestamp = int(reqid2timestamp(event.Event.Header.Reqid))
            if timestamp < self.startTimestamp or timestamp > self.endTimestamp:
                return
            host = event.Event.Header.Addr
            req = EVENT_TO_REQUEST[event.EventClassId]
            if event.EventClassId != EV_CAPTCHA_CHECK:
                resp = EVENT_TO_RESPONSE[event.EventClassId]
                success = '1'
            else:
                resp = 'OK' if event.Event.Success else '302'
                success = self.correctCaptchaSuccess if event.Event.Success else self.wrongCaptchaSuccess
            color = RESPONSE_TO_COLOR[resp]
            size = '1000'

            yield Record(str(timestamp), '|'.join([host, req, resp, size, str(int(success)), color]))
        except:
            pass

def main():
    parser = argparse.ArgumentParser(description = DESCRIPTION)
    parser.add_argument("start_datetime", help = 'Start date and time in format YYYMMDD or YYMMDD-hh:mm:ss')
    parser.add_argument("end_datetime", help = 'End date and time in format YYYMMDD or YYMMDD-hh:mm:ss', nargs = '?')
    parser.add_argument("-pw", "--pass-wrong-captcha-inputs", action = 'store_true', default = False, help = 'Incorrect captcha inputs go through the paddle during visualization')
    parser.add_argument("-pc", "--pass-correct-captcha-inputs", action = 'store_true', default = False, help = 'Correct captcha inputs go through the paddle during visualization')

    args = parser.parse_args()
    
    if args.end_datetime:
        start = parseDateTime(args.start_datetime)
        end = parseDateTime(args.end_datetime)
    else:
        start = datetime.strptime(args.start_datetime, "%Y%m%d")
        end = start + timedelta(days = 1) - timedelta(seconds = 1)
        
    dates = dateRange(start.date(), end.date())
    
    with TemporaryTable(TempTableName()) as tmpTable:
        mapper = CaptchaEventsMapper(startTimestamp = datetime2timestamp(start),
                                     endTimestamp = datetime2timestamp(end),
                                     correctCaptchaSuccess = not args.pass_correct_captcha_inputs, 
                                     wrongCaptchaSuccess = not args.pass_wrong_captcha_inputs)
        MapReduce.runMap(mapper, srcTables = EventLogTables(dates), dstTable = tmpTable.name, usingSubkey = False, sortMode = True)
        mr_print.PrintMrTable(tmpTable.name, sys.stdout, MR_SERVER)

if __name__ == "__main__":
    main();
