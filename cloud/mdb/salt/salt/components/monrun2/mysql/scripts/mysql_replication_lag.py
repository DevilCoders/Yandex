#!/usr/bin/env python
"""
Check MySQL replication lag
"""

import argparse
import math
import os
import psutil
import subprocess
import sys
import time
import json
import yaml
from mysql_util import die, is_offline, connect


DEFAULT_WARN_REPLICATION_LAG_LIMIT = 10
DEFAULT_CRIT_REPLICATION_LAG_LIMIT = 30
DEFAULT_MON_TABLENAME = 'repl_mon'

MYSYNC_INFO_PATH = '/var/run/mysync/mysync.info'
CATCHUP_FACTOR = 0.5
CATCHUP_TS_FILE = '/tmp/.backup_catchup_ts'


def is_low_free_space():
    try:
        with open(MYSYNC_INFO_PATH, 'r') as f:
            return json.loads(f.read()).get('low_space')
    except Exception:
        return False


def is_backup_or_catchup():
    with open(CATCHUP_TS_FILE, "w+") as fh:
        try:
            xbpid = int(subprocess.check_output(['pidof', 'xtrabackup']).strip())
            xbstart = psutil.Process(xbpid).create_time()
            endts = int(time.time() + (time.time() - xbstart) * CATCHUP_FACTOR)
            fh.write(str(endts))
            fh.seek(0)
            subprocess.check_call(['chown', 'monitor:monitor', CATCHUP_TS_FILE])
        except subprocess.CalledProcessError:
            pass
        try:
            endts = fh.read()
            if endts:
                return time.time() < int(endts)
        except ValueError:
            pass
    return False


def get_parser():
    parser = argparse.ArgumentParser()

    parser.add_argument('-w', '--warn',
                        type=int,
                        default=DEFAULT_WARN_REPLICATION_LAG_LIMIT,
                        help='Warning time limit (seconds)')

    parser.add_argument('-c', '--crit',
                        type=int,
                        default=DEFAULT_CRIT_REPLICATION_LAG_LIMIT,
                        help='Critical time limit (seconds)')
    return parser


def check_replication_lag(args, cursor):
    repl_lag_cmd = """
    SELECT UNIX_TIMESTAMP(CURRENT_TIMESTAMP(3)) - UNIX_TIMESTAMP(ts) AS lag_seconds FROM mdb_repl_mon
    """

    cursor.execute(repl_lag_cmd)
    lag_seconds = cursor.fetchone()[0]

    if lag_seconds >= args.crit:
        if is_low_free_space():
            die(1, "replication lag is {0} seconds, due to low free space".format(lag_seconds))
        if is_backup_or_catchup():
            die(1, "replication lag is {0} seconds, due to running backup of catchup after it".format(lag_seconds))
        die(2, "replication lag is {0} seconds".format(lag_seconds))

    if lag_seconds > args.warn:
        die(1, "replication lag is {0} seconds".format(lag_seconds))

    die(0, "replication lag is {0} seconds".format(lag_seconds))

def _main():
    args = get_parser().parse_args()

    try:
        with connect() as conn:
            cur = conn.cursor()
            cur.execute("SHOW SLAVE STATUS")
            row = cur.fetchone()
            if not row:
                die(0, 'master')
            check_replication_lag(args, cur)
    except Exception as e:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')


if __name__ == '__main__':
    _main()
