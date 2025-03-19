#!/usr/bin/python -u
# -*- coding: UTF-8 -*-

import sys
import datetime
import time

tskv_format = 'lenulca-access-log'
# [2021-02-26 00:18:11.664] 84ec59b534008c21 INFO ::1 "GET /get-music/16250/94dc36fd.109701116.5.14690772/320.mp3 HTTP/1.1" 200 0.064

while True:
    try:
        line = sys.stdin.readline()
        if line == '':
           break
        line_split = line.rstrip('\n').split()

        st = ';'.join(line_split[0].split(';')[0:3])
        # timestamp
        time_log = ' '.join(line_split[:2]).split(';')[-1][1:-1]
        time_datetime = datetime.datetime.strptime(
            time_log, "%Y-%m-%d %H:%M:%S.%f")
        timestamp = time.mktime(time_datetime.timetuple())
        timestamp = int(timestamp)

        # status
        status = line_split[-2]

        # request
        request = line_split[-4]

        # request_time
        request_time = line_split[-1]


        ip = line_split[4].replace('::ffff:', '')

        print "%s;tskv\ttskv_format=%s\tunixtime=%s\tip=%s\tstatus=%s\trequest=%s\trequest_time=%s" % (
            st, tskv_format, timestamp, ip, status, request, request_time)
    except:
        continue
