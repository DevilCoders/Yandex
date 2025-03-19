#!/usr/bin/env python

# -*- coding: UTF-8 -*-

import re
import sys

results_errors = {'parse': 0}

meters_codes = [
    '1xx',
    '2xx',
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
s_name = 'lenulca'

meters_urls = {
    'get': ['GET /get'],
    'del': ['GET /del'],
    'put': ['POST /put'],
    'ping': ['GET /ping'],
    'head': ['HEAD /get']
}

urls_meters = dict()
for meter, urls in meters_urls.items():
    for url in urls:
        urls_meters[url] = meter

meters_urls['others'] = None


results_count_urls = dict()
results_timings_urls = dict()
for mu_key in meters_urls:
    results_timings_urls[mu_key] = dict()
    for t_key in timings:
        results_timings_urls[mu_key][t_key] = []
    results_count_urls[mu_key] = dict()
    for mc_key in meters_codes:
        results_count_urls[mu_key][mc_key] = 0


# [2015-09-17 00:00:08.165380] ::ffff:95.108.252.3 "POST /put/70619.590552995.1447850221211002107010485274379 HTTP/1.0" result 200 "OK" "7698" 7531 168 "80P7kZmKEW2Z" 0.001
# [2015-09-17 00:00:08.206687] ::ffff:37.140.138.125 "GET /del/70167.1580707.574021698127543650526182978923 HTTP/1.0" result 200 "OK" "1052" 160 156 "80PAkZmK4KoZ" 0.004
# [2015-09-17 00:00:08.286759] ::ffff:127.0.0.1 "GET /get/69773.yadisk:5930300.34618585937250184524826348819?content_type=audio%2Fmpeg&raw HTTP/1.0" result 200 "OK" "3053255" 306 3053384 "wxljVZmK5CgZ" 10.140
# [2015-09-17 00:00:41.968455] ::ffff:127.0.0.1 "GET /ping?stid=68665.yadisk%3A69113063.2268987108214096604038905688774 HTTP/1.0" result 200 "OK" "" 178 203 "f0PZnamKEiEZ" 0.029
# [2015-09-17 09:44:21.330127] ::ffff:127.0.0.1 "HEAD /get/51030.yadisk:316736553.122258725212652820756072254151?content_type=application%2Fx-rar&raw HTTP/1.0" result 501 "NotImplemented" "" 405 73 "LiYJl95cJOs5" 0.000

# [2015-09-17 12:55:17.138100] 2a02:6b8:0:1a4f::434 "HEAD /gate/get/28559.yadisk:189350522.217243487051038291059699182507 HTTP/1.1" result 200 "OK" 194 147 "208a6f172efb2079" 0.044

index_re = re.compile(
    '"((?:HEAD|GET|POST) /(?:get|put|del|ping|status|top_traffic_stid)).* result (\d+) "([\w ]+)" "(\d{0,})" (\d+) (\d+) "[^"]+" (\d+\.\d+)$')


for line in sys.stdin:

    line = line.strip()
    matches = index_re.findall(line)

    if len(matches):
        b_url = matches[0][0]
        u_code = int(matches[0][1])
        b_description = matches[0][3]
        u_request_length = int(matches[0][4])
        u_bytes_sent = int(matches[0][5])
        u_time = matches[0][6]

        if b_url in urls_meters:
            u_url = urls_meters[b_url]
            if (u_bytes_sent < 1048576) or (u_request_length < 1048576):
                results_timings_urls[u_url]['lt1mb'].append(u_time)
            elif (u_bytes_sent >= 1048576) or (u_request_length >= 1048576):
                results_timings_urls[u_url]['ge1mb'].append(u_time)

            results_timings_urls[u_url]['all'].append(u_time)
        else:
            u_url = 'others'

        if u_code < 200:
            results_count_urls[u_url]['1xx'] += 1
        elif u_code < 300:
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

    else:
        print line
        results_errors['parse'] += 1

##

for u_url, result in sorted(results_count_urls.items()):
    for u_code, value in sorted(result.items()):
        results_count_codes[u_code] += value
        print("%s_%s_%s %d" % (s_name, u_url, u_code, value))

for u_code, value in sorted(results_count_codes.items()):
    print("%s_total_%s %d" % (s_name, u_code, value))


for u_url, result in sorted(results_timings_urls.items()):
    for t, value in sorted(result.items()):
        if u_url != 'ping':
            if len(value) == 0:
                value = ['0.001']
            print("%s_%s_%s_timings %s" %
                  (s_name, u_url, t, ' '.join(value)))


for error, value in sorted(results_errors.items()):
    print("%s_error_%s %d" % (s_name, error, value))


sys.exit(0)
