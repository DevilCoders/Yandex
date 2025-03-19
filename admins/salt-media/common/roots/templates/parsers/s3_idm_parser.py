#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys
from collections import defaultdict


def tskv_parse(logs):
    if logs[:9] == "timestamp":
        fields = ['timestamp', 'request_id', 'method', 'uri', 'status', 'bytes_received',
                  'error_code', 'request_time', 'error_level']
        row = dict(token.split("=", 1) for token in logs[9:].strip().split("\t")
                   if token.split("=", 1)[0] in fields)
        return row


def main(argv):

    result = defaultdict(dict)
    result['err'] = 0
    code_stats = {}
    err_level = {}

    logs = sys.stdin
    for line in logs:
        record = tskv_parse(line)
        # print record
        # {'status': '200',
        #  'bytes_received': '770',
        #  'bytes_sent': '1094169',
        #  'request_time': '0.273',
        #  'request_id': '18df6545f70da915'}
        try:
            if record.get('error_level'):
                error_level = record.get('error_level')
            if record.get('status'):
                code = int(record.get('status'))

            # http status for HTTP service
            if 200 <= code < 300:
                status = '2xx'
            elif 300 <= code < 400:
                status = '3xx'
            elif 400 <= code < 500:
                status = '4xx'
            elif 500 <= code:
                status = '5xx'
            code_stats.setdefault(status, 0)
            code_stats[status] += 1

            if status != '2xx':
                code_stats.setdefault('bad_http_status', 0)
                code_stats['bad_http_status'] += 1

            err_level.setdefault(error_level, 0)
            err_level[error_level] += 1

            if error_level != '"-"':
                err_level.setdefault('bad_error_level', 0)
                err_level['bad_error_level'] += 1

        except:
            result['err'] += 1

    # count code status for DB function
    for (k, v) in code_stats.items():
        print "{0} {1}".format(k, v)

    # count error_level for DB function
    for (k, v) in err_level.items():
        print "{0} {1}".format(k, v)

    print "error_parse %d" % int(result['err'])


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))
