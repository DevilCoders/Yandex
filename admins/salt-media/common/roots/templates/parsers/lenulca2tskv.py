#!/usr/bin/python -u
# -*- coding: UTF-8 -*-

import sys
import datetime
import time

tskv_format = 'lenulca-access-log'

while True:
    try:
        line = sys.stdin.readline()
        if line == '':
           break
    # for line in sys.stdin:
        line_split = line.rstrip('\n').split()

        st = ';'.join(line_split[0].split(';')[0:3])
        # timestamp
        time_log = ' '.join(line_split[:2]).split(';')[-1][1:-1]
        time_datetime = datetime.datetime.strptime(
            time_log, "%Y-%m-%d %H:%M:%S.%f")
        timestamp = time.mktime(time_datetime.timetuple())
        timestamp = int(timestamp)

        # status
        status = line_split[line_split.index('result') + 1]

        # request
        request = line_split[4]

        # request_time
        request_time = line_split[-1]

        bytes_sent = line_split[-3]
        bytes_recv = line_split[-4]

        ip = line_split[2].replace('::ffff:', '')

        print "%s;tskv\ttskv_format=%s\tunixtime=%s\tip=%s\tstatus=%s\trequest=%s\tbytes_sent=%s\tbytes_recv=%s\trequest_time=%s" % (
            st, tskv_format, timestamp, ip, status, request, bytes_sent, bytes_recv, request_time)
    except:
        continue
