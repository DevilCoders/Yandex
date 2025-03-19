#!/usr/bin/env python3
# -*- coding: utf-8 -%-
"""
Check wal-g backup status.
"""

import argparse
import logging
import os
import subprocess
import sys
import time
from datetime import datetime

DAY = 60 * 60 * 24

DEFAULT_DATE_FMT = '%Y-%m-%dT%H:%M:%SZ'


def main():
    """
    Program entry point.
    """
    logging.basicConfig(format='%(message)s')

    args_parser = argparse.ArgumentParser()
    args_parser.add_argument(
        '--warn-old',
        type=int,
        default=2,
        help='Warn if backup is older than this many days')
    args_parser.add_argument(
        '--crit-old',
        type=int,
        default=4,
        help='Crit if backup is older than this many days')
    args_parser.add_argument(
        '--warn-max-count',
        type=int,
        default=10,
        help='Warn if backup count is greater than given threshold')
    args = args_parser.parse_args()

    with open('/proc/uptime', 'r') as f:
        uptime = float(f.readline().split()[0])

    if os.path.exists('/etc/dbaas.conf'):
        stat_info = os.stat('/etc/dbaas.conf')
        dbaas_conf_ctime = time.time() - stat_info.st_ctime

        if uptime > dbaas_conf_ctime:
            uptime = dbaas_conf_ctime

    if uptime < DAY:
        die(0, 'OK')

    try:
        retcode, stdout, stderr = run('walg_wrapper.sh backup_list')
        if retcode:
            die(2, 'Can not get backup list: exit code {0}'.format(retcode))

        if not str.strip(stdout):
            die(2, 'No backups were found')

        backups = [l for l in stdout.split('\n') if l.startswith('stream_')]

        if len(backups) > args.warn_max_count:
            die(1, 'Too many backups exist: {0} > {1}'.format(len(backups), args.warn_max_count))
        elif len(backups) < 1:
            die(2, 'No backups were found')

        latest = backups[-1]
        backup_time = datetime.strptime(latest.split()[1], DEFAULT_DATE_FMT)
        delta = datetime.utcnow() - backup_time
        days = delta.days

        if days >= args.crit_old:
            die(2, 'Latest backup was more than {0} days ago: {1}'.format(days, latest))
        elif days >= args.warn_old:
            die(1, 'Latest backup was more than {0} days ago: {1}'.format(days, latest))

        die(0, 'OK')

    except Exception as exc:
        die(2, 'Unable to get status: {0}'.format(repr(exc)))

def run(cmd):
    """
    Run the command and return its output.
    """
    result = subprocess.run(cmd, shell=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.returncode, result.stdout.decode(), result.stderr.decode()


def die(status, message):
    """
    Emit status and exit.
    """
    print('{0};{1}'.format(status, message))
    sys.exit(0)


if __name__ == '__main__':
    main()
