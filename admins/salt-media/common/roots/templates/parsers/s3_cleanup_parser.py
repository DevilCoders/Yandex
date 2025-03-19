#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys
from collections import defaultdict


def tskv_parse(logs):
    if logs[:9] == "timestamp":
        fields = ['timestamp', 'rowcount', 'error_code', 'request_time', 'status_code']
        row = dict(
            token.split("=", 1) for token in logs[9:].strip().split("\t")
            if token.split("=", 1)[0] in fields)
        return row


def main(argv):

    result = defaultdict(dict)
    result['err'] = 0
    dict_code = {}
    dict_row = {}
    dict_timings_stats = {}
    bad_db_code = 0

    logs = sys.stdin
    for line in logs:
        record = tskv_parse(line)
        try:
            code = record.get('error_code')
            if not code:
                code = record.get('status_code')
            time = record.get('request_time')
            rowcount = record.get('rowcount', False)

            # Common errcode
            code_prefix = "db_" if rowcount else "http_"
            code_stats = dict_code.setdefault("S3_cleanup", {})
            code_stats.setdefault(code_prefix + code, 0)
            code_stats[code_prefix + code] += 1

            if rowcount and code != "00000":
                bad_db_code += 1

            if rowcount:
                # Common rowcount
                row_stats = dict_row.setdefault("S3_cleanup", {})
                row_stats.setdefault('rowcount', 0)
                row_stats['rowcount'] += int(rowcount)

            if rowcount:
                # Common timings
                timings_stats = dict_timings_stats.setdefault('S3_cleanup', {})
                total_times = timings_stats.setdefault('db_timings', [])
                total_times.append(time)
            else:
                # Common timings
                timings_stats = dict_timings_stats.setdefault('S3_cleanup', {})
                total_times = timings_stats.setdefault('mds_timings', [])
                total_times.append(time)

        except:
            result['err'] += 1

    # count errcode
    for (k, v) in dict_code.items():
        for status, count in v.items():
            print "{1} {2}".format(k, status, count)

    # count row
    for (k, v) in dict_row.items():
        for status, count in v.items():
            print "{1} {2}".format(k, status, count)

    # timings
    for (k, v) in dict_timings_stats.iteritems():
        for total, timings in v.items():
            print "{1} {2}".format(k, total, ' '.join(timings))

    if rowcount:
        print "bad_db_code %d" % bad_db_code
    print "error_parse %d" % int(result['err'])


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))
