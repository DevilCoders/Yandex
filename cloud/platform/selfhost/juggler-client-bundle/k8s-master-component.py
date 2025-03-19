#!/usr/bin/env python2

import json
import urllib2
import datetime


COMPONENTS_URL = "http://localhost:18880/components"
REQUIRED_RUNNING_TIME = datetime.timedelta(seconds=15)


class CompStatus:
    RUNNING = "RUNNING"


def query_comps():
    resp = urllib2.urlopen(COMPONENTS_URL).read()
    return json.loads(resp)["components"]


def parse_status_age(age):
    """Parses protobuf's Duration JSON representation
    (SA docs for google.protobuf.Duration)"""
    age = age.rstrip('s')
    sec = float(age)
    return datetime.timedelta(milliseconds=sec * 1000)


def check_component(name):
    comps = query_comps()

    comp = None
    for curr in comps:
        if curr["name"] == name:
            comp = curr
            break
    if comp is None:
        return False, "component is missing"

    if comp["status"] != CompStatus.RUNNING:
        return False, "component is not running"

    status_age = parse_status_age(comp["statusAge"])
    if status_age < REQUIRED_RUNNING_TIME:
        return False, "component is started too recently"

    return True, "OK"

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--component', required=True, help='component name to check')
    parser.add_argument('--service',
                        help='juggler service for report (derived from component by default)')
    args = parser.parse_args()
    if args.service is None:
        args.service = "k8s-{}".format(args.component)

    try:
        healthy, message = check_component(args.component)
    except urllib2.URLError as ex:
        healthy, message = False, ex.reason
    except Exception as ex:
        healthy, message = False, ex.message

    code = 0 if healthy else 2
    print "PASSIVE-CHECK:{};{};{}".format(args.service, code, message)
