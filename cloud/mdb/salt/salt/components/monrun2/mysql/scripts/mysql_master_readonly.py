#!/usr/bin/env python
"""
Check if MySQL master is writable
"""

import math
import os
import subprocess
import sys
import yaml
import json
from mysql_util import die, is_offline, connect

MYSYNC_INFO_PATH = '/var/run/mysync/mysync.info'

def is_low_free_space():
    try:
        with open(MYSYNC_INFO_PATH, 'r') as f:
            return json.loads(f.read()).get('low_space')
    except Exception:
        return False


def _main():
    try:
        with connect() as conn:
            cur = conn.cursor()
            cur.execute("SHOW SLAVE STATUS")
            row = cur.fetchone()
            if row:
                die(0, 'replica')
            cur.execute("SELECT @@read_only")
            is_ro = int(cur.fetchone()[0])
            if is_ro:
                if is_low_free_space():
                    die(1, 'master is read-only due to low free space')
                else:
                    die(2, 'master is read-only')
    except Exception:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')
    die(0, 'OK')


if __name__ == '__main__':
    _main()
