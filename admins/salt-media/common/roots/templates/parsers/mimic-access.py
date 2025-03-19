#!/usr/bin/env python

# -*- coding: UTF-8 -*-

import re
import sys

results_errors = {'parse': 0}

meters_codes = [
    '2xx',
    '3xx',
    '404',
    '499',
    '4xx',
    '5xx',
    'unknown',
    'all'
]

timings = [
    'all'
]

results_count_codes = dict()
for sp_key in meters_codes:
    results_count_codes[sp_key] = 0

# service name (for report)
s_name = 'spacemimic'

meters_urls = {
    'mds_stid': ['GET /get'],
    'mds_stid_service': ['GET /get'],
    'mulca_stid_lrc': ['GET /get'],
    'mulca_stid_lrc_service': ['GET /get'],
    'ping': ['GET /ping'],
    'getmds': ['GET /get-']
}

urls_meters = dict()
for meter, urls in meters_urls.items():
    for url in urls:
        urls_meters[url] = meter

meters_urls['others'] = None


results_count_urls = dict()
results_timings_urls = dict()
results_bytes = dict()
for spu_key in meters_urls:
    results_timings_urls[spu_key] = dict()
    for t_key in timings:
        results_timings_urls[spu_key][t_key] = []
    results_count_urls[spu_key] = dict()
    for sp_key in meters_codes:
        results_count_urls[spu_key][sp_key] = 0
    results_bytes[spu_key] = dict()
    results_bytes[spu_key]['bytes_sent'] = 0

## [2017-11-27 01:21:14.747] 1aa7b637ee7dbf31 INFO 127.0.0.1 "HEAD /get/320.yadisk:559061653.E764582:771685471124216651335795828270?service=disk_downloader_mulcagate HTTP/1.1" 200 0.005

index_re = re.compile(
    '((?:INFO|ERROR|DEBUG|WARNING)) (::1|127.0.0.1|2a02.+) "((?:GET|POST|HEAD) /(?:get-|get|ping)).* HTTP/1.\d+" (\d+) (\d+\.\d+)$')

stid_mds_re = re.compile(
    '/get/\d+\.(agodin|(yadisk\:\d+))\.E.*')

stid_mulca_re = re.compile(
    '/get/\d+\.yadisk\:\d+\.\d+')

stid_service_re = re.compile('.*service=([_\w\d-]+)')

for line in sys.stdin:

    line = line.strip()
    matches = index_re.findall(line)

    if len(matches):

        query = line.rsplit(' ')[6]

        b_url = matches[0][2]
        u_level = matches[0][0]
        u_code = int(matches[0][3])
        u_time = matches[0][4]

        if query:
            if stid_mds_re.match(query):
		if stid_service_re.search(query):
			u_url = "mds_stid_service"
			results_timings_urls[u_url]['all'].append(u_time)
		else:
			u_url = "mds_stid"
			results_timings_urls[u_url]['all'].append(u_time)
            elif stid_mulca_re.match(query):
		if stid_service_re.search(query):
			u_url = "mulca_stid_lrc_service"
			results_timings_urls[u_url]['all'].append(u_time)
		else:
			u_url = "mulca_stid_lrc"
			results_timings_urls[u_url]['all'].append(u_time)
            else:
                u_url = "getmds"
                results_timings_urls[u_url]['all'].append(u_time)


        #if b_url in urls_meters:
        #    u_url = urls_meters[b_url]

        #    results_timings_urls[u_url]['all'].append(u_time)

        #else:
        #    u_url = 'others'

        if u_code < 300:
   	    results_count_urls[u_url]['2xx'] += 1
        elif u_code < 400:
            results_count_urls[u_url]['3xx'] += 1
        elif u_code == 499:
            results_count_urls[u_url]['499'] += 1
        elif u_code == 404:
            results_count_urls[u_url]['404'] += 1
        elif u_code < 500:
            results_count_urls[u_url]['4xx'] += 1
        elif u_code < 600:
            results_count_urls[u_url]['5xx'] += 1
        else:
            results_count_urls[u_url]['unknown'] += 1

        results_count_urls[u_url]['all'] += 1

    else:
        results_errors['parse'] += 1


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
