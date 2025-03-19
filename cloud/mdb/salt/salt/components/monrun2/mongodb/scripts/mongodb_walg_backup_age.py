#!/usr/bin/env python3
# -*- coding: utf-8 -%-
"""
Check wal-g backup status.
"""

import iso8601
import argparse
import logging
import subprocess
import sys
from datetime import datetime, timezone


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
        help='Warn if last backup is older than this many days')
    args_parser.add_argument(
        '--crit-old',
        type=int,
        default=4,
        help='Crit if last backup is older than this many days')
    args_parser.add_argument(
        '--warn-max-count',
        type=int,
        default=10,
        help='Warn if backup count is greater than given threshold')
    args = args_parser.parse_args()

    try:
        retcode, stdout, stderr = run('walg_wrapper.sh backup_list -v')
        if retcode:
            die(2, 'Can not get backup list: exit code {0}'.format(retcode))

        if not str.strip(stdout):
            die(2, 'No backups were found')

        # stream_20210806T030113Z 2021-08-06T06:05:01+03:00 1628218873.1  1628219101.46 2195143203 false     {"backup_id":"mdbbhk39d1hnogt8qdrq","shard_name":"rs01"}
        backups = [l for l in stdout.split('\n') if l.startswith('stream_') and l.split()[5] == 'false']

        if len(backups) > args.warn_max_count:
            die(1, 'Too many automated backups exist: {0} > {1}'.format(len(backups), args.warn_max_count))
        elif len(backups) < 1:
            die(2, 'No backups were found')

        backup_name, backup_time, *_ = backups[-1].split()
        delta = datetime.now(timezone.utc) - iso8601.parse_date(backup_time)
        days = delta.days

        if days >= args.crit_old:
            die(2, 'Latest backup was more than {0} days ago: {1}'.format(days, backup_name))
        elif days >= args.warn_old:
            die(1, 'Latest backup was more than {0} days ago: {1}'.format(days, backup_name))

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

