#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys
import re
from collections import defaultdict


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option(
        "--print_error_parse", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--db", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--access", help="[ %default ]", default=False, action='store_true')
    parser.add_option(
        "--output", help="[ Default - %default ]", default='common')
    (options, arguments) = parser.parse_args(argv)
    if (len(arguments) > 1):
        raise Exception("Tool takes no arguments")
    return options


def tskv_parse(logs):
    if logs[:9] == "timestamp":
        fields = ['timestamp', 'request_id', 'bucket', 'remote_addr', 'schema', 'method',
                  'uri', 'status', 'bytes_received', 'bytes_sent', 'function', 'rowcount',
                  'error_code', 'attempt', 'request_time']
        row = dict(token.split("=", 1) for token in logs[9:].strip().split("\t") if token.split("=", 1)[0] in fields)
        return row


#TODO use parse_request
def parse_request(line, status, db=False, access=False):
    try:
        if db:
            return db_parse(line)
        elif access:
            return mds_request(line, vhost, cache, mds)
        elif request:
            return resizer_request(line)
        else:
            print line
    except:
        return None


def main(argv):
    # Ангумента
    options = parseArgs(argv)

    result = defaultdict(dict)
    result['err'] = 0
    dict_code = {}
    dict_attempt = {}
    dict_row = {}
    dict_timings_stats = {}
    code_none_count = 0

    s3_code_re = re.compile('^S3.*')
    logs = sys.stdin
    for line in logs:
        record = tskv_parse(line)
        # record = {
        # 'function': 'add_object',
        # 'attempt': '1',
        # 'request_time': '0.048',
        # 'rowcount': '1',
        # 'request_id': 'c12d4c7083d35d25',
        # 'error_code': '00000',
        # 'schema': 'v1_code'}
        try:
            function = record.get('function')
            bucket = record.get('bucket')
            if bucket == '-':
                bucket = 'unknown'
            bucket_function = '{}.{}'.format(bucket, function)
            code = record.get('error_code')
            s3_code_match = s3_code_re.match(code)
            attempt = record.get('attempt')
            time = record.get('request_time')
            rowcount = record.get('rowcount')

            retry_and_good_codes = ['40001', '40P01', '23505', '25006', '00000']

            # Common errcode for DB function
            code_stats = dict_code.setdefault('global_db' + '.' + function, {})
            if code in retry_and_good_codes or s3_code_match:
                code_stats.setdefault(code, 0)
                code_stats[code] += 1
            else:
                code_stats.setdefault('bad_code_' + code, 0)
                code_stats['bad_code_' + code] += 1

            # Count None error code for monitoring and alerting.
            if code == "None":
                code_none_count += 1

            # Errcode for bucket DB function
            code_stats = dict_code.setdefault(bucket_function, {})
            if code in retry_and_good_codes or s3_code_match:
                code_stats.setdefault(code, 0)
                code_stats[code] += 1
            else:
                code_stats.setdefault('bad_code_' + code, 0)
                code_stats['bad_code_' + code] += 1

            # Common  attempt for DB function
            attempt_stats = dict_attempt.setdefault('global_db.' + function, {})
            attempt_stats.setdefault(attempt, 0)
            attempt_stats[attempt] += 1

            # Bucket attempt for DB function
            attempt_stats = dict_attempt.setdefault(bucket_function, {})
            attempt_stats.setdefault(attempt, 0)
            attempt_stats[attempt] += 1

            # Common rowcount for DB function
            row_stats = dict_row.setdefault('global_db.' + function, {})
            row_stats.setdefault('rowcount', 0)
            row_stats['rowcount'] += int(rowcount)

            # Bucket rowcount for DB function
            row_stats = dict_row.setdefault(bucket_function, {})
            row_stats.setdefault('rowcount', 0)
            row_stats['rowcount'] += int(rowcount)

            # Common timings for DB function
            func_timings_stats = dict_timings_stats.setdefault('global_db.' + function, {})
            func_total_times = func_timings_stats.setdefault('timings', [])
            func_total_times.append(time)

            # Bucket timings for DB function
            func_timings_stats = dict_timings_stats.setdefault(bucket_function, {})
            func_total_times = func_timings_stats.setdefault('timings', [])
            func_total_times.append(time)

        except:
            result['err'] += 1

    # count errcode for DB function
    for (k, v) in dict_code.items():
        for status, count in v.items():
            print "{0}.{1} {2}".format(k, status, count)

    # count attempt for DB function
    for (k, v) in dict_attempt.items():
        for status, count in v.items():
            print "{0}_attempt_{1} {2}".format(k, status, count)

    # count row for DB function
    for (k, v) in dict_row.items():
        for status, count in v.items():
            print "{0}_{1} {2}".format(k, status, count)

    # timings for DB
    for (k, v) in dict_timings_stats.iteritems():
        for total, timings in v.items():
            print "{0}_{1} {2}".format(k, total, ' '.join(timings))

    print "global_db.bd_code_None %d" % code_none_count
    print "error_parse %d" % int(result['err'])


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))
