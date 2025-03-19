#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Simple script for yandex graphite system"""

from __future__ import print_function
import sys
import socket
import shutil
import argparse
import collections

import yaml
import pyjolokia


HOSTNAME = socket.getfqdn()
MONRUN_FILE = "/tmp/monrun-jolokia"


def clean_name(string):
    "clean metrics name"
    if string.startswith('clusterid='):
        return ''
    if string.startswith('port=2'):
        return ''
    string = string.split("=")
    return string[-1:][0]


def get_name(string, attr):
    """
    Automatically build name for graphite
    metric from jolokia metric name
    """

    attr = str(attr).lower()
    if ":" in string:
        root, name = string.split(":")
    else:
        root, name = "", string

    name = name.replace(".", "_").replace(" ", "_").replace("-", "_").lower()
    name = ".".join([clean_name(nme) for nme in name.split(",") if clean_name(nme)])
    name = "{0}.{1}.{2}".format(root, name, attr)
    return name


def parse_response(resp, app_name):
    "Parse jolokia response in to simple list of strings"
    metrics = []
    for res in resp:
        if res.get('status', 0) != 200:
            sys.stderr.write("failed {0}\n".format(res))
            continue

        if res['request']['type'] == 'read':
            name = "{0}.{1}".format(
                app_name,
                get_name(res['request']['mbean'], res['request']['attribute'])
            )

            value = res['value']
            if '*' in res['request']['mbean']:
                #  u'org.mongodb.driver:clusterId=5718edf4a8f9c6f63a1284f4,
                #  host=rcmnd-back04e.music.yandex.net,
                #  port=27017,type=ConnectionPool': {u'CheckedOutCount': 0},
                set_hosts = collections.defaultdict(int)
                for host in value:
                    name = "{0}.{1}".format(
                        app_name,
                        get_name(host.replace(HOSTNAME, ""), value[host].keys()[0]))
                    set_hosts[name] += value[host].values()[0]
                for key, val in set_hosts.iteritems():
                    metrics.append("{0} {1}".format(key, val))
                continue
            elif isinstance(value, dict):
                value = value.get("used")
            metrics.append("{0} {1}".format(name, value))
        elif res['request']['type'] == 'exec':
            name = "{0}.{1}".format(
                app_name,
                get_name(res['request']['mbean'], res['request']['operation'])
            )
            value = res['value']
            if res['request']['operation'] == 'dumpAllThreads':
                statuses = {}
                for thread in value:
                    status = thread['threadState'].lower()
                    statuses[status] = statuses.get(status, 0) + 1
                for status in statuses:
                    metrics.append("{0}.{1} {2}".format(name, status, statuses[status]))
            elif res['request']['operation'] == 'findDeadlockedThreads':
                deadlocked = len(value or [])
                metrics.append("{0}.deadlocked {1}".format(name, deadlocked))

    return metrics


def graphite():
    "Send metrics to stdout and write monrun file"
    conf = yaml.load(open("/etc/monitoring/jolokia.yaml"))
    for app in conf:
        try:
            name = app['name']
        except KeyError:
            sys.stderr.write("No name key for app {0}\n".format(app))
            continue

        port = int(app.get("port", 8778))
        j = pyjolokia.Jolokia('http://localhost:{0}/jolokia/'.format(port))

        for mname, mattr in app.get("read", []):
            if '{HOSTNAME}' in mname:
                mname = mname.replace("{HOSTNAME}", HOSTNAME)
            j.add_request(type='read', mbean=mname, attribute=mattr)
        for operation in app.get("exec", []):
            args = {}
            if operation == 'status_count':
                args = {
                    'type': 'exec',
                    'mbean': 'java.lang:type=Threading',
                    'operation': 'dumpAllThreads',
                    'arguments': ['false', 'false']
                }
            elif operation == 'deadlock_count':
                args = {
                    'type': 'exec',
                    'mbean': 'java.lang:type=Threading',
                    'operation': 'findDeadlockedThreads',
                    'arguments': []
                }
            if args:
                j.add_request(**args)

        response = parse_response(j.getRequests(), name)
        print("\n".join(response))
        try:
            # touch monrun file
            with open(MONRUN_FILE, "a") as mfile:
                pass
            with open(MONRUN_FILE + "-tmp", "a+") as mfile:
                mfile.write("\n".join(response) + '\n')
            shutil.move(MONRUN_FILE, MONRUN_FILE + ".prev")
            shutil.move(MONRUN_FILE + "-tmp", MONRUN_FILE)
        except Exception as exc:  # pylint: disable=broad-except
            sys.stderr.write("Failed to write monrun files {0}\n".format(exc))


def monrun():
    "Check saved metrics and print status"
    parser = argparse.ArgumentParser(description="Jolokia monrun")
    parser.add_argument(
        '--type',
        help="Type of counter (supported: counter,gauge) default=gauge "
             "details https://github.com/statsite/statsite#protocol",
        default="gauge"
    )
    parser.add_argument('--description', help="append check description")
    parser.add_argument('monrun', help="back compatibility item, ignored but required")
    parser.add_argument('key', help="jolokia metric key")
    parser.add_argument('threshold', help="threshold value", type=float)
    args = parser.parse_args()

    msg = "2;{0} NoData".format(args.key)
    try:
        mdata = dict([l.split() for l in open(MONRUN_FILE).readlines()])
        prev_mdata = dict([l.split() for l in open(MONRUN_FILE + ".prev").readlines()])
    except:  # pylint: disable=bare-except
        print("1;No data for monrun check, probably daemon not responding")
        sys.exit(0)

    try:
        for key in mdata:
            if args.key not in key:
                continue

            value = float(mdata[key])
            if args.type == "counter":
                value -= float(prev_mdata[key])
            msg = "0;OK"
            if value > args.threshold:
                msg = "2;{0} {1} > {2}".format(args.key, value, args.threshold)
                if args.description:
                    msg = "{0} {1}".format(msg, args.description)
    except Exception as exc:  # pylint: disable=broad-except
        msg = "2;Failed to parse jolokia data, {0}".format(exc)
    print(msg)

if __name__ == '__main__':

    if sys.argv[1:]: # have args
        monrun()
    else:
        graphite()

    # gr = socket.socket()
    # try:
    #     gr.connect(('localhost', 42000))
    #     gr.sendall("\n".join(response))
    # except (socket.error, socket.timeout) as err:
    #     print "socket error {0}".format(err)
