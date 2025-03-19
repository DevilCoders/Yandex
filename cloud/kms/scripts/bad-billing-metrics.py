#!/usr/bin/env python

import argparse
import calendar
import ssl
import time
import urllib2

ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE

# Converts string in MSK time (e.g. "2020-12-31T23:59:00") to unix timestamp.
def parse_msk_time(s):
    # MSK = UTC + 3h
    return int(calendar.timegm(time.strptime(args.from_time, "%Y-%m-%dT%H:%M:%S"))) - 3 * 3600

def request_for_metrics_type(preprod, metrics_source, metrics_type, from_time, to_time, token):
    if preprod:
        base_url = "https://billing.private-api.cloud-preprod.yandex.net:16465"
    else:
        base_url = "https://billing.private-api.cloud.yandex.net:16465"
    topic = "yc{0}/billing-kms-{1}".format("/preprod" if preprod else "", metrics_source)
    url = "{0}/billing/v1/private/{1}Metrics?sourceName=topic-{2}-0&startTime={3}&endTime={4}".format(base_url, metrics_type, topic, start_time, end_time)
    print("Requesting URL " + url)

    headers = {
        "Content-Type": "application/json",
        "X-YaCloud-SubjectToken": token
    }
    req = urllib2.Request(url, None, headers)
    response = urllib2.urlopen(req, context=ctx)
    return response.read()

parser = argparse.ArgumentParser()
parser.add_argument("--preprod", help="use PREPROD instead of PROD",
                    action="store_true")
parser.add_argument("--source", help="metrics source: api or storage", required=True)
parser.add_argument("--from", help="find bad metrics from time", required=True, dest='from_time')
parser.add_argument("--to", help="find bad metrics up to time (by default from time + 1hr)", dest='to_time')
parser.add_argument("--token", help="IAM token", required=True)
args = parser.parse_args()

start_time = parse_msk_time(args.from_time)
if args.to_time:
    end_time = parse_msk_time(args.to_time)
else:
    end_time = start_time + 3600

for metrics_type in ["discarded", "expired", "unparsed"]:
    print("Got {0} metrics:".format(metrics_type))
    print(request_for_metrics_type(args.preprod, args.source, metrics_type, start_time, end_time, args.token))
