#!/usr/bin/env python
"""
Check if switchover is performed in reasonable time
"""

import datetime
import os
import subprocess
import sys
import yaml
import dateutil.parser
import json

from mysql_util import die


TIMEOUT_WARN = datetime.timedelta(minutes=10)
TIMEOUT_CRIT = datetime.timedelta(hours=1)


MYSYNC_INFO_PATH = '/var/run/mysync/mysync.info'


def _main():
    try:
        with open(MYSYNC_INFO_PATH, 'r') as f:
            mysync_info = json.loads(f.read())

        switchover = mysync_info.get('switch')
        if switchover is None:
            die(0, 'OK')
        start = dateutil.parser.parse(switchover.get('initiated_at'))
        # to avoid install pytz for timezones
        now = subprocess.check_output(['date', '--iso-8601=seconds'])
        now = dateutil.parser.parse(now)
        if start + TIMEOUT_CRIT < now:
            die(2, 'mysync switchover takes too long')
        if start + TIMEOUT_WARN < now:
            die(1, 'mysync switchover takes too long')
    except Exception as e:
        die(1, 'failed to check switchover')
    die(0, 'OK')

if __name__ == '__main__':
    _main()
