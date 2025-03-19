#!/usr/bin/env python

# -*- coding: UTF-8 -*-

import re
import sys


def tskvParse(record):
    if record[:5] != "tskv\t":
        return None
    fields = [
        'status', 'request', 'bytes_recv', 'bytes_sent',
        'method', 'request_time'
    ]
    row = dict(
        token.split("=", 1) for token in record[5:].strip().split("\t")
        if token.split("=", 1)[0] in fields)
    if not ignore_req(row['request']):
        return row
    else:
        return 'Ignore'


def ignore_req(request):
    ignore = [
        '/ping', '/', '/gate/stats'
    ]
    if request in ignore:
        return True

    return False


results_errors = {'parse': 0}

meters_codes = [
    '1xx',
    '2xx',
    '2xx_disconnect',
    '2xx_reading_failed',
    '3xx',
    '4xx',
    '5xx',
    'unknown',
    'all'
]

timings = [
    'lt1mb',
    'ge1mb',
    'all'
]

results_count_codes = dict()
for mc_key in meters_codes:
    results_count_codes[mc_key] = 0

# service name (for report)
s_name = 'mulcagate'
service_re = re.compile('(?<=service=)[_\w\d-]+')

meters_urls = {
    'get': ['GET /gate/get'],
    'del': ['GET /gate/del'],
    'custom': ['GET /gate/custom-get'],
    'put': ['POST /gate/put'],
    'ping': ['GET /ping'],
    'headget': ['HEAD /gate/get'],
    'dist-info': ['GET /gate/dist-info']
}

urls_meters = dict()
for meter, urls in meters_urls.items():
    for url in urls:
        urls_meters[url] = meter

meters_urls['others'] = None


results_count_urls = dict()
results_timings_urls = dict()
results_bytes = dict()
for mu_key in meters_urls:
    results_timings_urls[mu_key] = dict()
    for t_key in timings:
        results_timings_urls[mu_key][t_key] = []
    results_count_urls[mu_key] = dict()
    for mc_key in meters_codes:
        results_count_urls[mu_key][mc_key] = 0
    results_bytes[mu_key] = dict()
    results_bytes[mu_key]['bytes_sent'] = 0
    results_bytes[mu_key]['request_length'] = 0


for line in sys.stdin:

    line = line.strip()
    tskv = tskvParse(line)
    if tskv is None:
        results_errors['parse'] += 1
    elif tskv == 'Ignore':
        pass
    else:
        u_code = int(tskv['status'])
        u_request_length = int(tskv['bytes_recv'])
        u_bytes_sent = int(tskv['bytes_sent'])
        u_time = float(tskv['request_time'])

        b_description = ''

        url = "{} {}".format(tskv['method'], tskv['request'])
        split_url = url.split('/', 3)
        b_url = "{}/{}".format(split_url[0], '/'.join(split_url[1:3]))
        request = tskv['request']

        service = service_re.search(line)
        if service is None:
            service = 'unknown'
        else:
            service = service.group(0)
        ignore_services = ['sherlock']

        if service not in ignore_services:
            if b_url in urls_meters:
                u_url = urls_meters[b_url]
                if (u_bytes_sent < 1048576) and (u_request_length < 1048576):
                    results_timings_urls[u_url]['lt1mb'].append(u_time)
                elif (u_bytes_sent >= 1048576) or (u_request_length >= 1048576):
                    results_timings_urls[u_url]['ge1mb'].append(u_time)

                results_timings_urls[u_url]['all'].append(u_time)
                results_bytes[u_url]['bytes_sent'] += u_bytes_sent
                results_bytes[u_url]['request_length'] += u_request_length

            else:
                u_url = 'others'

            if u_code < 200:
                results_count_urls[u_url]['1xx'] += 1
            elif u_code < 300:
                if b_description == 'Client was disconnected':
                    results_count_urls[u_url]['2xx_disconnect'] += 1
                elif b_description.startswith('Reading failed'):
                    results_count_urls[u_url]['2xx_reading_failed'] += 1
                else:
                    results_count_urls[u_url]['2xx'] += 1
            elif u_code < 400:
                results_count_urls[u_url]['3xx'] += 1
            elif u_code < 500:
                results_count_urls[u_url]['4xx'] += 1
            elif u_code < 600:
                results_count_urls[u_url]['5xx'] += 1
            else:
                results_count_urls[u_url]['unknown'] += 1

            results_count_urls[u_url]['all'] += 1

##

for u_url, result in sorted(results_count_urls.items()):
    for u_code, value in sorted(result.items()):
        results_count_codes[u_code] += value
        print("%s_%s_%s %d" % (s_name, u_url, u_code, value))

for u_url, result in sorted(results_bytes.items()):
    for u_code, value in sorted(result.items()):
        print("%s_%s_%s %d" % (s_name, u_url, u_code, value))


for u_code, value in sorted(results_count_codes.items()):
    print("%s_total_%s %d" % (s_name, u_code, value))


for u_url, result in sorted(results_timings_urls.items()):
    for t, value in sorted(result.items()):
        if u_url != 'ping':
            if len(value) == 0:
                value = ['0.001']
            print("%s_%s_%s_timings %s" %
                  (s_name, u_url, t, ' '.join([str(x) for x in value])))

for error, value in sorted(results_errors.items()):
    print("%s_error_%s %d" % (s_name, error, value))

sys.exit(0)
