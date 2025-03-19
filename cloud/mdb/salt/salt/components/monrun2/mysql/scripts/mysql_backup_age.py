#!/usr/bin/env python3
# -*- coding: utf-8 -%-
"""
Check if wal-g backup is not too old
"""

import argparse
import os
import sys
import subprocess
import time

from datetime import datetime, timezone, timedelta
from dateutil import parser
from mysql_util import die, is_offline, connect


UPTIME_FILE = "/proc/uptime"
UPGRADE_FILE = "/tmp/.mysql_upgrade_done"
DELAY = 1.5 * 3600 * 24


def uptime():
    with open("/proc/uptime") as fh:
        return int(float(fh.readline().split()[0]))


def time_since_upgrade():
    try:
        stat = os.stat(UPGRADE_FILE)
        return int(time.time() - stat.st_ctime)
    except FileNotFoundError:
        return None
    
    
def _main():
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument(
        '-w',
        '--warn',
        type=int,
        default=2,
        help='warning number of days')
    args_parser.add_argument(
        '-c',
        '--crit',
        type=int,
        default=4,
        help='critical number of days')

    args = args_parser.parse_args()

    try:
        with connect() as conn:
            cur = conn.cursor()
            cur.execute("SHOW SLAVE STATUS")
            row = cur.fetchone()
            if row:
                die(0, 'replica')
    except Exception:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')

    since_boot = uptime()
    if since_boot < DELAY:
        die(0, 'less than 1.5 day since boot')
    since_upgrade = time_since_upgrade()
    if since_upgrade is not None and since_upgrade < DELAY:
        die(0, 'less than 1.5 day since upgrade')

    proc = subprocess.Popen([
        '/usr/bin/timeout', '90',
        '/usr/bin/wal-g-mysql', '--config', '/etc/wal-g/wal-g.yaml', 'backup-list',
    ],stdout=subprocess.PIPE,stderr=subprocess.DEVNULL)
    proc.wait()
    if proc.returncode != 0:
        die(1, 'failed to list backups')

    latest_backup_delta = float('inf')
    now = datetime.now(timezone.utc)
    for line in iter(proc.stdout.readline, b''):
        tokens = line.rstrip().split()
        if len(tokens) < 2:
            continue
        try:
            backup_time = parser.parse(tokens[1].decode("utf-8"))
        except ValueError:
            continue
        delta = (now - backup_time).total_seconds()
        if delta < latest_backup_delta:
            latest_backup_delta = delta
    if latest_backup_delta == float('inf'):
        die(2, 'No backups found')
    elif latest_backup_delta > timedelta(days=args.crit).total_seconds():
        die(2, 'Latest backup was more than %d days' % (args.crit))
    elif latest_backup_delta > timedelta(days=args.warn).total_seconds():
        die(1, 'Latest backup was more than %d days' % (args.warn))
    else:
        die()


if __name__ == '__main__':
    _main()
