#!/usr/bin/env python

import re
import time
import datetime

re_date = re.compile(r'(\d+)\/([^\/]+)\/(\d+):(\d+):(\d+):(\d+)\ ([\+\-]\d{2})(\d{2})')
re_time = re.compile(r'(\d{1,2}):(\d{1,2}):(\d{1,2})')


def TimeToDateStr(tim):
    t = time.localtime(int(tim))
    return time.strftime("%Y%m%d", t)

def TimeToTimeStr(tim):
    t = time.localtime(int(tim))
    return time.strftime("%H:%M:%S", t)

def TimeFromStr(str):
    r = re_time.search(str)
    if r:
        return datetime.time(int(r.group(1)), int(r.group(2)), int(r.group(3)))
    else:
        return None


def DateObjFromDateStr(str):
    st = time.strptime(str, "%Y%m%d")
    return datetime.date(st.tm_year, st.tm_mon, st.tm_mday)

def DateTimeObjFromDateStr(str):
    st = time.strptime(str, "%Y%m%d")
    return datetime.datetime(st.tm_year, st.tm_mon, st.tm_mday, 0, 0, 0)

def DateTimeToTimestamp(dateTime):
    return int(time.mktime(dateTime.timetuple()))


def DateObjToStr(dat):
    return dat.strftime("%Y%m%d")
