#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json

import requests

STATUS = []


def curl():
    api = "http://localhost:9000/"
    try:
        req = requests.get(api)
        req.raise_for_status()
        response = req.text
        print_metrics(response)
    except Exception:
        pass


def dump(path, obj, dumper):
    metric = ""
    try:
        keys = path.split(".", 1)
        if len(keys) > 1:
            if keys[0] == "*":
                res = []
                for k in obj:
                    res.append(dump(keys[1], obj[k], float))
                metric = dumper(res)
            else:
                metric = dump(keys[1], obj[keys[0]], dumper)
        else:
            metric = dumper(obj[path])
    except Exception:  # NOQA
        # print("<combainer_monitoring_failed_as>: {0}".format(str(e)))
        pass
    return metric


def add(name, value):
    if str(value):
        STATUS.append("{0} {1}".format(name, value))


def print_metrics(resp):
    try:
        stats = json.loads(resp)
        add("nofile", dump("Files.Open", stats, str))
        add("goroutines", dump("GoRoutines", stats, str))
        add("clients", dump("Clients", stats, len))
        add("aggregate.total", dump("Clients.*.AggregateTotal", stats, sum))
        add("aggregate.failed", dump("Clients.*.AggregateFailed", stats, sum))
        add("aggregate.success", dump("Clients.*.AggregateSuccess", stats, sum))
        add("parsing.total", dump("Clients.*.ParsingTotal", stats, sum))
        add("parsing.failed", dump("Clients.*.ParsingFailed", stats, sum))
        add("parsing.success", dump("Clients.*.ParsingSuccess", stats, sum))
    except Exception:  # NOQA
        # status = "<combainer/monitoring/failed/as> {0}".format(str(e))
        pass


if __name__ == '__main__':
    curl()
    for s in STATUS:
        print(s)
