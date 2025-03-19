#!/usr/bin/env python3
# -*- coding: utf-8 -%-
"""
Check wal-g oplog-push status.
"""

import argparse
import json
import logging
import requests
import subprocess
import sys
from datetime import datetime


DEFAULT_DATE_FMT = '%Y-%m-%dT%H:%M:%SZ'


def main():
    """
    Program entry point.
    """
    logging.basicConfig(format='%(message)s')

    args_parser = argparse.ArgumentParser()
    args_parser.add_argument(
        '--warn-lag',
        type=int,
        default=600,
        help='Warn if lag is greater')
    args_parser.add_argument(
        '--crit-lag',
        type=int,
        default=1800,
        help='Crit if lag is greater')
    args = args_parser.parse_args()

    try:
        url = 'http://127.0.0.1:8090/stats/oplog_push'
        r = requests.get('http://127.0.0.1:8090/stats/oplog_push')
        r.raise_for_status()
        data = r.json()
        if data['status'] == 'standby':
            die(0, 'Waiting for primary')

        lag = data['mongo']['last_known_maj_ts']['TS'] - data['archived']['last_ts']['TS']

        msg = 'Archiving lag: {0}'.format(lag)
        if lag >= args.crit_lag:
            die(2, msg)
        elif lag >= args.warn_lag:
            die(1, msg)

        die(0, msg)

    except Exception as exc:
        die(2, 'Unable to get lag: {0}'.format(repr(exc)))


def die(status, message):
    """
    Emit status and exit.
    """
    print('{0};{1}'.format(status, message))
    sys.exit(0)


if __name__ == '__main__':
    main()
