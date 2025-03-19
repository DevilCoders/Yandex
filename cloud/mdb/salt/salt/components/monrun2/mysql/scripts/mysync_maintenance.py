#!/usr/bin/env python
"""
Check if maintenance entered and leaved in reasonable time
"""

import datetime
import os
import subprocess
import sys
import yaml
import dateutil.parser
import json

from mysql_util import die

ENTER_TIMEOUT = datetime.timedelta(seconds=1800) # dbaas_worker/providers/mysync.py
LEAVE_TIMEOUT_WARN = datetime.timedelta(hours=2)
LEAVE_TIMEOUT_CRIT = datetime.timedelta(hours=6)


MYSYNC_INFO_PATH = '/var/run/mysync/mysync.info'


def _main():
    try:
        with open(MYSYNC_INFO_PATH, 'r') as f:
            mysync_info = json.loads(f.read())

        maintenance = mysync_info.get('maintenance')
        if maintenance is None:
            die(0, 'OK')
        start = dateutil.parser.parse(maintenance.get('initiated_at'))
        # to avoid install pytz for timezones
        now = subprocess.check_output(['date', '--iso-8601=seconds'])
        now = dateutil.parser.parse(now)
        if start + ENTER_TIMEOUT < now and not maintenance.get('mysync_paused'):
            die(2, 'mysync didn\'t enter maintenance')
        if start + LEAVE_TIMEOUT_CRIT < now:
            die(2, 'mysync maintenance takes too long')
        if start + LEAVE_TIMEOUT_WARN < now:
            die(1, 'mysync maintenance takes too long')
    except Exception as e:
        die(1, 'failed to check maintenance')
    die(0, 'OK')


if __name__ == '__main__':
    _main()
