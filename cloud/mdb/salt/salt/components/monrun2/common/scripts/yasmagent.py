#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Check if local yasmagent is ok
"""

from __future__ import absolute_import, print_function, unicode_literals

import os
import sys
import time

import requests


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('%s;%s' % (status, message))
    sys.exit(0)


def _main():
    if not os.path.exists('/etc/init.d/yasmagent'):
        die()

    try:
        data = \
            requests.get('http://localhost:11003/json/').json()
        if data.get('status', '').lower() != 'ok':
            die(1, 'Wrong status: {status}'.format(
                status=data.get('status', '')))
        die()
    except Exception as exc:
        die(1, 'Unable to check: %s' % repr(exc))


if __name__ == '__main__':
    _main()
