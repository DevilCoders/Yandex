#!/usr/bin/python

import argparse
import json
import os
import sys
import subprocess
import time
from mysql_util import die, is_offline, connect

MB = 1024 * 1024

DEFAULT_CRIT_BINLOG_TRESHOLD = 1073741824

DEFAULT_WARN_BINLOG_TRESHOLD = int(DEFAULT_CRIT_BINLOG_TRESHOLD / 2)

WALG_CACHE_FILE = '/home/mysql/.walg_mysql_binlogs_cache'


def get_parser():
    parser = argparse.ArgumentParser()

    parser.add_argument('-w', '--warn',
                        type=int,
                        default=DEFAULT_WARN_BINLOG_TRESHOLD,
                        help='Warning size of unarchived binlog (bytes)')

    parser.add_argument('-c', '--crit',
                        type=int,
                        default=DEFAULT_CRIT_BINLOG_TRESHOLD,
                        help='Critical size of unarchived binlog (bytes)')
    return parser


def list_binlogs():
    res = []
    cmd = ['sudo', '/bin/ls', '-l', '--time-style=+%s', '/var/lib/mysql']
    stdout = subprocess.check_output(cmd, shell=False)
    prefix = 'mysql-bin-log-'
    now = time.time()
    for line in stdout.split(os.linesep):
        if not line or line.startswith('total'):
            continue
        line = line.split()
        name = line[-1]
        ctime = int(line[-2])
        size = line[4]
        if name.startswith(prefix):
            res.append((name, size, int(now - ctime)))
    return res


def binlog_gt(b1, b2):
    b1 = int(b1.split(".")[-1])
    b2 = int(b2.split(".")[-1])
    return b1 > b2


def _main():
    args = get_parser().parse_args()

    last_binlog = ''
    size = 0

    if os.path.exists(WALG_CACHE_FILE):
        with open(WALG_CACHE_FILE, 'r') as file:
            data = json.load(file)
            last_binlog = data['LastArchivedBinlog']

    try:
        with connect() as conn:
            cur = conn.cursor()
            cur.execute("SHOW SLAVE STATUS")
            row = cur.fetchone()
            if row:
                die(0, 'replica')
            binlogs = list_binlogs()
            if len(binlogs) > 0 and binlog_gt(last_binlog, binlogs[-1][0]):
                die(2, 'binlogs are not uploading, invalid cache')
            for row in binlogs:
                if binlog_gt(row[0], last_binlog) and row[2] > 60:
                    size += int(row[1])
    except Exception:
        if is_offline():
            die(1, 'Mysql is offline')
        die(1, 'Mysql is dead')

    if size >= args.crit:
        die(2, '{size}MB of binlog unarchived'.format(size=int(size / MB)))
    elif size >= args.warn:
        die(1, '{size}MB of binlog unarchived'.format(size=int(size / MB)))
    else:
        die()


if __name__ == '__main__':
    _main()
