#!/usr/bin/env python2

import argparse
import json
import os
import ssl
import urllib2


# Config file example content
#
# {
#     "envoy_front": {
#         "port": 80,
#         "scheme": "http",
#         "header-host": null,
#         "host": "localhost",
#         "path": "/ping"
#         "timeout": 3
#     }
# }


def create_req(instance):
    req = urllib2.Request("{}://{}:{}/{}".format(
        instance.get("scheme", "http"),
        instance.get("host", "localhost"),
        instance.get("port", 80),
        instance.get("path", "/ping")[1:]
    ))
    header_host = instance.get("header-host")
    if header_host is not None:
        req.add_header('Host', header_host)
    return req


def create_exit_message(srv, code, msg):
    return "PASSIVE-CHECK:{};{};{}".format(srv, code, msg)


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--service', required=True,
                        help='service name to check; check parameters stored in config file')
    parser.add_argument('--config-file',
                        default="{}/platform-http-checks.json".format(
                            os.path.dirname(os.path.realpath(__file__))),
                        help='service name to check; check parameters stored in config file')
    return parser.parse_args()


def check_resp(r):
    if r.getcode() != 200:
        raise urllib2.URLError("%s raise %d" % (r.geturl(), r.getcode()))
    return r


if __name__ == "__main__":
    args = parse_arguments()
    service = args.service

    with open(args.config_file) as f:
        d = json.load(f)

    instance = d.get(service)

    if instance is None:
        code = 1
        msg = "No check found"
    else:
        try:
            ctx = ssl.create_default_context()
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE
            check_resp(urllib2.urlopen(create_req(instance),
                                       timeout=instance.get("timeout", 3),
                                       context=ctx))
        except Exception as e:
            code = 2
            msg = e
        else:
            code = 0
            msg = "OK"
    print(create_exit_message(service, code, msg))
