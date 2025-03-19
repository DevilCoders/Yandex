#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import requests
import urllib
from urlparse import urlparse, parse_qs
import logging
import re
import subprocess

log = logging.getLogger('downloader.count-reporter')
log.setLevel(logging.DEBUG)
_format = logging.Formatter("%(levelname)s\t%(asctime)s\t\t%(message)s")
_handler = logging.FileHandler('/var/log/downloads_counter.log')
_handler.setFormatter(_format)
_handler.setLevel(logging.DEBUG)
log.addHandler(_handler)

app = "downloader.count-reporter"
dst_url = "http://mpfs.disk.yandex.net/service/kladun_download_counter_inc"

def tskv_parse(line):
    result = {}
    for x in line[5:].split('\t'):
        spl = x.split('=', 1)
        try:
            key = spl[0]
            value = spl[1]
        except IndexError:
            continue

        result[key] = value

    return result

mtail='/usr/bin/mymtail.sh'
alog='/var/log/nginx/downloader/access.log'
ticket='/run/ticker_for_che.tvm2'
rerange=re.compile("^bytes=[1-9][0-9]*-[0-9]*")

if  os.path.exists(alog) and os.path.exists(mtail) and os.path.exists(ticket):
    fin = os.popen("%s %s %s" % (mtail,alog,app), "r")

    for line in fin:
        tskv = tskv_parse(line)
        o = urlparse(tskv['request'])
        query = parse_qs(o.query)
        if 'hash' in query:
            match = re.search(rerange, tskv['range'])
            if match:
                count = 0
            else:
                count = len(query['hash'])
            url = "%s?hash=%s&bytes=%s&count=%s"  % (dst_url, urllib.quote(query['hash'][0]), tskv['bytes_sent'], count)
            with open(ticket) as f:
                tvm2ticket = f.read()
            headers = {'User-Agent': app, 'X-Ya-Service-Ticket': tvm2ticket }
            resp = requests.get(url, timeout=3, headers=headers)
            if 'request_id' in tskv:
                request_id = tskv['request_id']
            else:
                request_id = "NULL"
            if resp.status_code == requests.codes.okay:
                log.info('url=%s return_code=%s request_id=%s' % (url, resp.status_code, request_id))
            else:
                log.error('Timeout or error to %s %s request_id=%s' % (url, resp.status_code, request_id))
