#!/usr/bin/env python

# -*- coding: UTF-8 -*-

import re
import sys

s_name = 'libmulca'

# traffic

traffic = {
    'up': '( PUT|STREAM_PUT) .* finish Ok .* bytes_transferred=(\d+)',
    'down': '(GET|GET_BLOB|GET_HEADER|GET_TEXT|GET_HEADER_TEXT|GET_PART).* bytes_transferred=(\d+)'
}

result_traffic = dict()
for key in traffic.keys():
    result_traffic[key] = 0

# traffic

# timings

timings = {
    'put': 'PUT .* finish Ok .* time=(\d+)ms'
}

timings_t = {
    'put': [10, 50, 100, 300, '300+']
}

result_timings = dict()
for key in timings.keys():
    result_timings[key] = dict()
    result_timings[key]['all'] = []
    for x in timings_t[key]:
        result_timings[key][x] = 0

# timings

# requesrs
# readers - https://github.yandex-team.ru/mulca/libmulca/blob/master/src/storage_readers.cc
requests = {
    'put': ['PUT .* finish Ok'],
    'put_error': ['libmulca_error: .* (PUT|STREAM_PUT)'],
    'del': ['DEL .* STATUS_OK'],
    'del_error': ['libmulca_error: .* DEL'],
    'get': ['( GET| GET_BLOB| GET_HEADER| GET_TEXT| GET_HEADER_TEXT| GET_PART) .* STATUS_OK',
            'libmulca_info: .* READ_CLOSE \((GET|GET_BLOB|GET_HEADER|GET_TEXT|GET_HEADER_TEXT|GET_PART)_READER\) .* (END_OF_STREAM|STATUS_OK)'],
    'get_404': ['( GET| GET_BLOB| GET_HEADER| GET_TEXT| GET_HEADER_TEXT| GET_PART) .* MESSAGE_NOT_FOUND',
                'libmulca_error: .* READ_CLOSE \((GET|GET_BLOB|GET_HEADER|GET_TEXT|GET_HEADER_TEXT|GET_PART)_READER\) .* MESSAGE_NOT_FOUND'],
    'get_error': ['( GET| GET_BLOB| GET_HEADER| GET_TEXT| GET_HEADER_TEXT| GET_PART) .* (ERROR_GET_MESSAGE|UNIT_NOT_FOUND)',
                  'libmulca_error: .* READ_CLOSE \((GET|GET_BLOB|GET_HEADER|GET_TEXT|GET_HEADER_TEXT|GET_PART)_READER\) .* (ERROR_GET_MESSAGE|UNIT_NOT_FOUND)'],
}

result_requests = dict()
for key in requests.keys():
    result_requests[key] = 0

# requesrs

# error
error_parse = 0
ignore_error = ['libmulca_info: version:SCS', 'libmulca_info: naming_store::http_abstract_reloader::get_http_content: host=',
                'libmulca_info: units_list_t:', 'libmulca_info: naming_store::reload_loop_: units loaded ok', 'chunk_write Ok', 'next_try Ok']

# error

for line in sys.stdin:

    parse = False
    line = line.strip()

    for key, regexp in traffic.items():
        matches = re.compile(regexp).findall(line)
        if len(matches):
            result_traffic[key] += int(matches[0][-1])

    for key, regexp in timings.items():
        matches = re.compile(regexp).findall(line)
        if len(matches):
            time = int(matches[0])
            x = timings_t[key]

            if time <= x[0]:
                result_timings[key][x[0]] += 1
            elif time <= x[1]:
                result_timings[key][x[1]] += 1
            elif time <= x[2]:
                result_timings[key][x[2]] += 1
            elif time <= x[3]:
                result_timings[key][x[3]] += 1
            else:
                result_timings[key][x[4]] += 1

            norm_time = str(round(0.001 * time, 3))
            result_timings[key]['all'].append(norm_time)

    for key, regexps in requests.items():
        for regexp in regexps:
            matches = re.compile(regexp).findall(line)
            if len(matches):
                parse = True
                result_requests[key] += 1

    if not parse:
        flap = False
        for x in ignore_error:
            if x in line:
                flap = True
        if not flap:
            error_parse += 1
##

print("%s_parse_error %d" % (s_name, error_parse))

for key, value in sorted(result_traffic.items()):
    print("%s_traffic_%s %d" % (s_name, key, value))

for key, value in sorted(result_timings.items()):
    for x, count in sorted(value.items()):
        if x == 'all':
            print("%s_timings_%s_%s %s" % (s_name, key, x, ' '.join(count)))
        else:
            print("%s_timings_%s_%s %d" % (s_name, key, x, count))

for key, value in sorted(result_requests.items()):
    print("%s_status_%s %d" % (s_name, key, value))

sys.exit(0)
