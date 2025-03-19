#!/usr/bin/env python3

"""
Simple juggler downtime manager
Support set/list/remove juggler downtimes
Many defaults and simple jctl like user interface
"""

import argparse
import json
import logging as log
import os
import re
import socket
import sys
import time
from datetime import datetime
from types import SimpleNamespace

import requests

SOURCE = 'jdowntime.py'
LOG_LEVEL_NAMES = ["debug", "info", "warning", "error", "critical"]
KNOWN_METHODS = ["set", "remove", "get"]


def ts2time(unixtime):
    """
    ts2time converts unixtime to human readable time format
    """
    return datetime.fromtimestamp(int(unixtime)).strftime('%Y-%m-%d %H:%M:%S')


def time_to_seconds(string):
    """
    time_to_seconds convert strings like 7d, 1h, 10s or 3m in to seconds interval
    """
    match = re.match(r'^\+?(?P<num>\d+)(?P<spec>[\s\w]*)', str(string).strip())
    if not match:
        raise ValueError("Can not parse %s" % string)
    num = int(match.group('num'))
    spec = match.group('spec').strip().lower()
    if not spec or spec.startswith('s'):
        pass
    elif spec.startswith('m'):
        num = num * 60
    elif spec.startswith('h'):
        num = num * 3600
    elif spec.startswith('d'):
        num = num * 3600 * 24
    else:
        raise ValueError("Can not parse %s" % string)
    return num


def preparse_args(parser):
    """
    parse_args insert supported downtime operations variants into help strings
    """
    usage = parser.format_usage()
    usage = usage.split()[1:]
    usage.insert(1, "({})".format("|".join(KNOWN_METHODS)))
    parser.usage = " ".join(usage)

    params = sys.argv[1:]
    if not params:
        parser.print_help()
        sys.exit(1)

    alt_name_set = ["add"]
    alt_name_remove = ["rm", "del"]
    alt_name_get = ["list"]
    allowed = KNOWN_METHODS + alt_name_set + alt_name_remove + alt_name_get

    for method in allowed:
        if method.startswith(params[0]):
            params[0] = method

    if params[0] not in allowed:
        parser.print_help()
        sys.exit(1)
    else:
        if params[0] in alt_name_set:
            params[0] = "set"
        if params[0] in alt_name_remove:
            params[0] = "remove"
        if params[0] in alt_name_get:
            params[0] = "get"

    return params[0], params[1:]


class Downtime(object):
    """
    Downtime class contains main logic and methods abount downtimes
    """

    def __init__(self, opts):
        if isinstance(opts, dict):
            opts = SimpleNamespace(opts)

        self.opts = opts
        self.opts.service = getattr(opts, "service", None)
        self.opts.start = getattr(opts, "start", None)
        if self.opts.start:
            self.opts.start = time_to_seconds(self.opts.start)
        self.opts.desc = getattr(opts, "desc", None)
        if not getattr(opts, "host", None):
            self.opts.host = socket.getfqdn()
            log.debug("Host not set, use hostname: %s", self.opts.host)
        if not getattr(opts, "stop", None):
            log.warning("Stop parameter not valid '%s', force default='12 hours'",
                        getattr(opts, "stop", None))
            self.opts.stop = 3600 * 12
        else:
            self.opts.stop = time_to_seconds(self.opts.stop)
        if not getattr(opts, "endpoint", None):
            self.opts.endpoint = 'juggler-api.search.yandex.net'
            log.debug("Use default juggler api endpoint: juggler-api.search.yandex.net")
        if not getattr(opts, "config", None):
            self.opts.config = "/etc/yandex/jdowntime.conf"
            log.debug("Use default config: /etc/yandex/jdowntime.conf")

        if os.path.exists(self.opts.config):
            self.conf = json.load(open(self.opts.config))
        else:
            log.error("Config '%s' does not exists", self.opts.config)
            self.conf = {}

    def do_dt(self, method="set"):
        """do_dt do downtime operations"""

        if method not in KNOWN_METHODS:
            msg = "Unknown method: {}".format(method)
            log.critical(msg)
            raise NameError(msg)

        headers = {'Content-Type': 'application/json'}
        if 'token' in self.conf:
            headers['Authorization'] = 'OAuth {}'.format(self.conf['token'])

        filters = {"host": self.opts.host}
        if self.opts.service:
            filters["service"] = self.opts.service
        data = {"filters": [filters]}

        if method == "set":
            data.update({
                "source": SOURCE,
                "end_time": int(time.time()) + self.opts.stop,
                "description": str(self.opts.desc),
            })
            if self.opts.start:
                data["start_time"] = int(time.time()) + self.opts.start
                data["end_time"] += self.opts.start

        if method == "remove":
            ids = self.opts.ids
            if not ids:
                ids = self.get_downtime_ids(filters, headers)
            if not ids:
                log.info("No downtimes for removal")
                return
            data = {"downtime_ids": ids}

        if method == "get":
            data["page_size"] = 100

        data = json.dumps(data)
        log.debug("Request data %s", data)
        url = self.opts.url_template.format(self.opts.endpoint, method)
        log.info("Request url: %s", url)
        result = requests.post(url, data=data, headers=headers, timeout=5)
        log.info("Request completed with status %s", result.status_code)
        log.debug("Response body: %s", result.text)
        result.raise_for_status()

        if method == "get":
            self.print_downtimes(result.text)

    def get_downtime_ids(self, filters, headers):
        url = self.opts.url_template.format(self.opts.endpoint, "get")
        # page_size 100 enough for everyone
        filters = dict(filters)
        filters["source"] = SOURCE
        data = {"filters": [filters], "page_size": 100}
        result = requests.post(url, data=json.dumps(data), headers=headers, timeout=5)
        log.info("Query downtime ids completed with status %s", result.status_code)
        log.debug("Response body: %s", result.text)
        result.raise_for_status()
        downtimes = result.json()
        ids = []
        for dt in downtimes["items"]:
            log.info("Downtime to remove %s", dt["filters"])
            ids.append(dt["downtime_id"])
        return ids

    def set(self):
        """Proxy method to set downtime"""
        self.do_dt("set")

    def remove(self):
        """Proxy method to remove downtime"""
        self.do_dt("remove")

    def get(self):
        """Proxy method to remove downtime"""
        self.do_dt("get")

    @staticmethod
    def print_downtimes(resp):
        """print_downtimes pretty print json response"""
        red = '\033[91m'
        green = '\033[92m'
        yellow = '\033[93m'
        endc = '\033[0m'
        tpl = "{0}{{0:<10}}\t{1}{{1}}{2}".format(red, yellow, endc)
        downtimes = json.loads(resp)
        if not downtimes["items"]:
            print("no downtimes")
        else:
            print("Total {}, Shown {}".format(downtimes["total"], len(downtimes["items"])))
        for item in downtimes["items"]:
            print("{:_<40}{}".format(green, endc))
            print(tpl.format("Downtime ID:", item["downtime_id"]))
            print(tpl.format("Description:", item["description"]))
            if item["filters"][0]["service"]:
                print(tpl.format("Service:", item["filters"][0]["service"]))
            print(tpl.format("Host:", item["filters"][0]["host"]))
            print(tpl.format("Duration: from", ts2time(item["start_time"])), end=" -> ")
            print(tpl.format("to", ts2time(item["end_time"])))

    def __enter__(self):
        self.do_dt("set")

    def __exit__(self, exc_type, exc_value, traceback):
        if exc_value:
            log.getLogger("__exit__").exception("Abnormal exit from with")
        self.do_dt("remove")


class HelpFormatter(argparse.ArgumentDefaultsHelpFormatter):
    def __init__(self, *args, **kwargs):
        kwargs["width"] = 120
        super().__init__(*args, **kwargs)


def main():
    """Main"""

    fqdn = socket.getfqdn()
    argp = argparse.ArgumentParser(
        description='Simple juggler downtime manager', formatter_class=HelpFormatter)
    argp.add_argument('-host', help="host/group name", default=fqdn)
    argp.add_argument('-ids', help="Downtime ids to remove", nargs="+")
    argp.add_argument('-desc', help="Downtime description",
                      default="Downtime by jdowntime.py from {}@{}".format(
                          os.environ.get("LC_USER", os.environ.get("USER", "unknown")),
                          socket.getfqdn()))
    argp.add_argument('-service', help="Downtime specified service")
    argp.add_argument('-stop', help="For how long (example '+1day|30m', default='+24h')",
                      default=3600 * 24)
    argp.add_argument('-start', help="When to start (example \"'1 day'|30|2hours\")",
                      default=0)
    argp.add_argument('-config', help="config path", default="/etc/yandex/jdowntime.conf")
    argp.add_argument('-endpoint', help="Api endpoint", default='juggler-api.search.yandex.net')
    argp.add_argument('-force-over-http', help="You are sure?", action='store_true')
    argp.add_argument('-l', '--log', metavar="log",
                      help="Logging level " + ",".join(LOG_LEVEL_NAMES),
                      choices=LOG_LEVEL_NAMES, type=str, default="error")

    method, arguments = preparse_args(argp)
    args = argp.parse_args(arguments)
    fmt = '%(asctime)s %(levelname)-8s %(funcName)18s.%(lineno)d: %(message)s'
    log.basicConfig(level=getattr(log, args.log.upper()), format=fmt, datefmt="%F %T")

    args.url_template = 'https://{}/v2/downtimes/{}_downtimes'
    if args.force_over_http:
        args.url_template = 'http://{}/v2/downtimes/{}_downtimes'

    dtm = Downtime(args)
    dtm.do_dt(method=method)


if __name__ == '__main__':
    main()
