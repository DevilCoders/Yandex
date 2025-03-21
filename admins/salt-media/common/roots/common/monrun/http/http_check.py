#!/usr/bin/env python3
'''http and https local check'''

import os
import sys
from pprint import pprint as pp
import requests
from yaml import load

DEBUG = bool(os.getenv("HTTP_CHECK_DEBUG", False))

if not DEBUG:   # 2>/dev/null
    FHANDLER = open('/dev/null', 'w')
    sys.stderr = FHANDLER


def debug(data, desc="debug message"):
    """
    enable debug mode
    :param data: config, dict
    :return: debug message
    """
    if DEBUG:
        pp({desc: data})


def load_config():
    """
    load config from file
    :return: config, dict
    """
    conf = {"http": [], "https": []}
    try:
        conf_file = "/etc/monitoring/http_check.yaml"
        if len(sys.argv) > 2:
            conf_file = sys.argv[2]
        conf = load(open(conf_file))
    except:  # pylint: disable=broad-except
        err = sys.exc_info()
        print("1;Exception {0[0].__name__}: {0[1]}!".format(err))
        sys.exit(0)

    debug(conf, "Raw checks data")
    for k in conf:
        for item in conf[k]:
            if "proto" not in item:
                item["proto"] = k
            if "handle" not in item:
                item["handle"] = '/ping'
            else:
                if not item["handle"].startswith("/"):
                    item["handle"] = '/{0}'.format(item["handle"])
            if "port" not in item:
                item["port"] = ''
            else:
                item["port"] = ":{0}".format(item["port"])
    debug(conf, "Prepared checks data")
    return conf


def make_uri(data):
    """
    create uri
    :param data: config, dict
    :return: uri, string
    """
    uri = "{0[proto]}://localhost{0[port]}{0[handle]}".format(data)
    debug(uri, "Maked uri")
    return uri


def make_headers(data):
    """
    return all headers from config
    :param data: config, dict
    :return: headers, dict
    """
    headers = data.get("headers")
    if "Host" not in headers:
        headers["Host"] = "localhost"

    return data.get("headers")


def ping(item):
    """
    send custom request
    :param item: dict
    :return: error if exists
    """
    error = ""
    uri = make_uri(item)
    headers = make_headers(item)
    debug(headers, "Headers content")
    try:
        resp = requests.get(uri, allow_redirects=False,
                            verify=item.get('verify', False), headers=headers)
        if resp.status_code != requests.codes.ok:
            error = "{0.url} {1}: {0.status_code} - {0.reason}".format(resp,
                                                                       headers)
        debug("{0.url} {1}: {0.status_code} - {0.reason}".format(resp,
                                                                 headers))
    except:  # pylint: disable=broad-except
        err = sys.exc_info()
        error = "{0}: {1[0].__name__}: {1[1]}".format(uri, err)
        debug(error)
    return error


def main():
    '''main function'''
    config = load_config()

    mode = 'http'
    if len(sys.argv) > 1:
        mode = sys.argv[1]

    config_mode = config[mode]
    msg = map(ping, config_mode)

    if any(msg):
        debug(msg, "Msg list")
        message = "2;{0}".format(",".join(filter(None, msg)))
    elif not msg:
        message = "0;Checks not configured, Ok."
    else:
        handles = map(lambda x: x['handle'], config_mode)
        message = "0; {0} Ok".format(','.join(handles))

    print(message)

if __name__ == '__main__':
    main()
