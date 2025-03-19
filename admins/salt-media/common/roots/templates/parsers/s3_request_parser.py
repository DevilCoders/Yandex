#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys
# import re
from collections import defaultdict


def parseArgs(argv):
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("--print_error_parse",
                      help="[ %default ]",
                      default=False,
                      action='store_true')
    parser.add_option("--db",
                      help="[ %default ]",
                      default=False,
                      action='store_true')
    parser.add_option("--access",
                      help="[ %default ]",
                      default=False,
                      action='store_true')
    parser.add_option("--output",
                      help="[ Default - %default ]",
                      default='common')
    (options, arguments) = parser.parse_args(argv)
    if (len(arguments) > 1):
        raise Exception("Tool takes no arguments")
    return options


def tskv_parse(logs):
    if logs[:9] == "timestamp":
        fields = [
            'timestamp', 'request_id', 'remote_addr', 'bucket', 'owner',
            'schema', 'request_method', 'method', 'uri', 'status',
            'bytes_received', 'bytes_sent', 'function', 'rowcount',
            'error_code', 'attempt', 'request_time', 'status_desc',
            'namespace', 'response_header_time'
        ]
        row = dict(
            token.split("=", 1) for token in logs[9:].strip().split("\t")
            if token.split("=", 1)[0] in fields)
        return row


# # TODO use parse_request
# def parse_request(line, status, db=False, access=False):
#     try:
#         if db:
#             return db_parse(line)
#         elif access:
#             return mds_request(line, vhost, cache, mds)
#         elif request:
#             return resizer_request(line)
#         else:
#             print(line)
#     except:
#         return None


def main(argv):
    # Ангумента
    # options = parseArgs(argv)

    result = defaultdict(dict)
    result['err'] = 0
    dict_code = {}
    dict_bytes_rcv = {}
    dict_bytes_sent = {}
    dict_timings_stats = {}
    dict_header_timings_stats = {}

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
            if record.get('request_method'):
                method = record.get('request_method')
            elif record.get('method'):
                method = record.get('method')
            function = record.get('namespace', False)
            if not function:
                function = record.get('bucket')
                function = function.replace('.', '_')
            if function == '-':
                function = 'unknown'
            if record.get('bytes_received'):
                bytes_rcv = record.get('bytes_received')
            if record.get('status'):
                code = record.get('status')
            if record.get('bytes_sent'):
                bytes_sent = record.get('bytes_sent')
            if record.get('request_time'):
                time = record.get('request_time')
            owner = record.get('owner', False)
            if owner == '-':
                owner = 'unknown'
            if owner:
                owner = 'owner_' + owner
            header_time = record.get('response_header_time', '')

            # http status for HTTP service
            if owner:
                code_stats = dict_code.setdefault('{}.{}.{}'.format(
                    function, owner, method), {})
            else:
                code_stats = dict_code.setdefault(function + "." + method, {})
            code_stats.setdefault(code, 0)
            code_stats[code] += 1

            # bytes_rcv for HTTP service
            if owner:
                bytes_rcv_stats = dict_bytes_rcv.setdefault(
                    '{}.{}'.format(function, owner), {})
            else:
                bytes_rcv_stats = dict_bytes_rcv.setdefault(function, {})
            bytes_rcv_stats.setdefault('bytes_rcv', 0)
            bytes_rcv_stats['bytes_rcv'] += int(bytes_rcv)

            # bytes_sent for HTTP service
            if owner:
                bytes_sent_stats = dict_bytes_sent.setdefault(
                    '{}.{}'.format(function, owner), {})
            else:
                bytes_sent_stats = dict_bytes_sent.setdefault(function, {})
            bytes_sent_stats.setdefault('bytes_sent', 0)
            bytes_sent_stats['bytes_sent'] += int(bytes_sent)

            # timings for HTTPT service
            if owner:
                func_timings_stats = dict_timings_stats.setdefault(
                    '{}.{}'.format(function, owner), {})
            else:
                func_timings_stats = dict_timings_stats.setdefault(
                    function, {})
            func_total_times = func_timings_stats.setdefault(
                method + '_timings', [])
            func_total_times.append(time)

            # header timings for HTTPT service
            if header_time:
                func_header_timings_stats = dict_header_timings_stats.setdefault(
                    function, {})
                func_header_total_times = func_header_timings_stats.setdefault(
                    method + '_header_response_timings', [])
                func_header_total_times.append(header_time)

        except:
            # print record
            result['err'] += 1

    # count errcode for DB function
    for (k, v) in dict_code.items():
        for status, count in v.items():
            print("{0}.{1} {2}".format(k, status, count))

    # bytes_rcv for service
    for (k, v) in dict_bytes_rcv.items():
        for status, count in v.items():
            if owner:
                print("{0}.{1} {2}".format(k, status, count))
            else:
                print("{0}_{1} {2}".format(k, status, count))

    # bytes_sent for service
    for (k, v) in dict_bytes_sent.items():
        for status, count in v.items():
            if owner:
                print("{0}.{1} {2}".format(k, status, count))
            else:
                print("{0}_{1} {2}".format(k, status, count))

    # timings for service
    for (k, v) in dict_timings_stats.iteritems():
        for total, timings in v.items():
            if owner:
                print("{0}.{1} {2}".format(k, total, ' '.join(timings)))
            else:
                print("{0}_{1} {2}".format(k, total, ' '.join(timings)))

    # header timings for service
    for (k, v) in dict_header_timings_stats.iteritems():
        for total, timings in v.items():
            print("{0}_{1} {2}".format(k, total, ' '.join(timings)))

    print("error_parse %d" % int(result['err']))


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))
