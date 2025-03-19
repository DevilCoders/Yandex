#!/usr/bin/python3

import collections
import os
import sys
import json
from datetime import datetime

from tabulate import tabulate
from termcolor import colored


def pretty_print_time_delta(delta):
    seconds = delta.seconds
    hours, remainder = divmod(seconds, 3600)
    minutes, seconds = divmod(remainder, 60)
    hours_str, minutes_str, seconds_str = "", "", ""
    if hours > 0:
        hours_str = "{}h".format(hours)  # for python2.7/3.5 compatibility
    if minutes > 0:
        minutes_str = "{}m".format(minutes)
    if (hours == 0 and minutes == 0) or seconds > 0:
        seconds_str = "{}s".format(seconds)
    return "{}{}{}".format(hours_str, minutes_str, seconds_str)


if len(sys.argv) != 1:
    raw = os.popen(
        "jclient-api print-events --status {} 2>/dev/null".format(sys.argv[1])
    ).read()
else:
    raw = os.popen("jclient-api print-events 2>/dev/null").read()
# sorting by service
result = {}
for j in json.loads(raw):
    result[j["service"]] = j
result = collections.OrderedDict(sorted(result.items()))
# make table
tabs = []
colors = {"OK": "green", "WARN": "yellow", "CRIT": "red"}
now = datetime.now()
for j in result:
    if result[j]["status"] in colors:
        status = colors[result[j]["status"]]
    else:
        status = "blue"

    age = datetime.strptime(result[j]["created"], "%a %b %d %H:%M:%S %Y")
    tabs.append(
        [
            colored(result[j]["service"], status),
            pretty_print_time_delta(now - age),
            colored(repr(result[j]["description"]), status),
        ]
    )
print(tabulate(tabs, headers=["Service", "Age", "Status"]))
