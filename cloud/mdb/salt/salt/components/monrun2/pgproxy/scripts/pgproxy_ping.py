#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, unicode_literals

import os
import sys

import requests

CLOSE_FILE = '/tmp/pgproxy_close'


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('%s;%s' % (status, message))
    sys.exit(0)


def _main():
    try:
        ping_response = requests.get('http://localhost:8080/ping', timeout=0.1)
        if ping_response.status_code == 200:
            die()
        elif os.path.exists(CLOSE_FILE):
            die(1, 'pgbouncer is closed because ' + CLOSE_FILE)
        else:
            die(2, '/ping returns ' + ping_response.content)
    except requests.Timeout:
        die(2, '/ping timeout')
    except Exception:
        die(2, '/ping does not work')


if __name__ == '__main__':
    _main()
