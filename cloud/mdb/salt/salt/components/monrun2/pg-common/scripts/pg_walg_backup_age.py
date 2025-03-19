#!/usr/bin/env python3
# -*- coding: utf-8 -%-
"""
Check if wal-g backup is not too old
"""

import argparse
import sys
import subprocess
from datetime import datetime, timezone, timedelta
from dateutil import parser


def die(status=0, message='OK'):
    """
    Emit status and exit
    """
    print('%s;%s' % (status, message))
    sys.exit(0)


def read_file_by_lines(path):
    """
    Returns iterator by lines of file
    """
    with open(path, 'r') as config:
        start = True
        line = ''
        while start or line:
            start = False
            line = config.readline()
            yield line


def _get_backup_push_timeout(walg_backup_push_conf_path):
    for line in read_file_by_lines(walg_backup_push_conf_path):
        tokens = line.split()
        if not tokens or tokens[0] != 'timeout':
            continue
        return int(tokens[2])


def get_uptime_seconds():
    for line in read_file_by_lines('/proc/uptime'):
        uptime_seconds, _ = line.split()
        return int(float(uptime_seconds))


def _main():
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument(
        '--conf',
        type=str,
        default='/etc/wal-g-backup-push.conf',
        help='Path to config for wal-g backup-push')
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

    proc = subprocess.Popen(
                [
                    '/usr/bin/timeout', '90',
                    '/usr/bin/wal-g', 'backup-list',
                    '--config', '/etc/wal-g/wal-g.yaml',
                ], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)

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
    timeout = _get_backup_push_timeout(args.conf)
    day = timedelta(days=1).total_seconds()
    warn_timeout = max(timeout, timedelta(days=args.warn).total_seconds())
    crit_timeout = max(timeout, timedelta(days=args.crit).total_seconds())

    # Uptime too small, host was stopped, show OK
    if get_uptime_seconds() < warn_timeout:
        die()

    # Call `wait`, cause need the command return code.
    # The documentation warns us about the possible deadlock which `Popen.wait` and `stdout=PIPE` may produce.
    # https://docs.python.org/3.6/library/subprocess.html#subprocess.Popen.wait
    # We shouldn't get it here cause earlier we read the full command stdout.
    proc.wait()
    if proc.returncode == 124:
        # If we don't get backups listing within timeout,
        # treat it as WARN instead of CRIT for empty backup list.
        die(1, 'wal-g command timeout')
    if proc.returncode != 0:
        die(1, 'failed to list backups')
    if latest_backup_delta == float('inf'):
        die(2, 'No backups found')
    elif latest_backup_delta > crit_timeout:
        die(2, 'Latest backup was more than %s days' % (crit_timeout // day))
    elif latest_backup_delta > warn_timeout:
        die(1, 'Latest backup was more than %s days' % (warn_timeout // day))
    else:
        die()


if __name__ == '__main__':
    _main()
