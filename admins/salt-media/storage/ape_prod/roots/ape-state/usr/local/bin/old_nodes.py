#!/usr/bin/env python
import requests
import time
import json

WARNING_DIFF = 180
CRITICAL_DIFF = 600

STATUS_OK = 0
STATUS_WARN = 1
STATUS_FAIL = 2

curl_cluster = 'http://localhost:8080/v1/cluster'
curl_status = 'http://localhost:8080/v1/status'


def report_status(status, message):
    """
    Renders status report for monrun in nagios format: <status>;<comment string>
    Immediately breaks execution to be sure we don't report two statuses from single script
    """
    print "{status};{comment}".format(status=status, comment=message)
    exit(0)


def render_comment(hosts, diff_threshold, max_hosts=None):
    """
    Renders comment report for check status line.
    May limit number of hosts listed in report to some maximum size to reduce message size
    """
    if max_hosts is not None and len(hosts) > max_hosts:
        hosts=hosts[:max_hosts]
        hosts.append("...")

    return "{hosts} feedback older than '{diff}' seconds".format(
        hosts=", ".join(hosts),
        diff=str(diff_threshold)
    )


try:
    r_cluster = requests.get(curl_cluster, timeout=10)
except:
    report_status(STATUS_WARN, "failed to load cluster info with http request")

try:
    r_status = requests.get(curl_status, timeout=10)
except:
    report_status(STATUS_WARN, "failed to load status info with http request")

try:
    leadership = json.loads(r_status.text)['leadership']
except:
    report_status(STATUS_WARN, "failed to decode status data: '{0}'".format(r_status.text))

master = []
for l in leadership:
    if l != '_unspec':
        if leadership[l] is True:
            master.append(l)

try:
    nodes = json.loads(r_cluster.text)['nodes']
except:
    report_status(STATUS_WARN, "failed to decode cluster data: '{0}'".format(r_cluster.text))

fail_hosts = []
warn_hosts = []

timestamp = int(time.time())
for n in nodes:
    if nodes[n]['cluster'] in master:
        if nodes[n]['timestamp'] is not None:
            diff = timestamp - nodes[n]['timestamp']
            if diff > CRITICAL_DIFF:
                fail_hosts.append(nodes[n]['hostname'])

            elif diff > WARNING_DIFF:
                warn_hosts.append(nodes[n]['hostname'])

# We have critical hosts
if len(fail_hosts) != 0:
    comment = render_comment(fail_hosts, CRITICAL_DIFF)

    if len(warn_hosts) != 0:
        comment += "; " + render_comment(warn_hosts, WARNING_DIFF, max_hosts=5)

    report_status(STATUS_FAIL, comment)

# We have only warning hosts
if len(warn_hosts) != 0:
    report_status(
        STATUS_WARN,
        render_comment(warn_hosts, WARNING_DIFF)
    )

# Everything is OK
report_status(STATUS_OK, "Ok")
