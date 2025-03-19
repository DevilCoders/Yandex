#!/usr/bin/env python
"""
Check if local redis replication lag is not too large
"""

import argparse
import json
import os
import sys
import time


from redis import StrictRedis


STATE_FILE = '{{ salt.mdb_redis.get_replication_state_file() }}'


class ReplicationState:
    def __init__(self, path=STATE_FILE):
        self.path = path

    def set(self, data=""):
        if not os.path.isfile(self.path):
            with open(self.path, 'w') as fobj:
                fobj.write(data)

    def clear(self):
        if os.path.isfile(self.path):
            os.remove(self.path)


replication_state = ReplicationState()


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def file_is_not_old_enough(path, timeout):
    if not os.path.exists(path):
        return True
    return time.time() - os.stat(path).st_mtime <= timeout


def process_state_file(path, timeout):
    if file_is_not_old_enough(path, timeout):
        return 0, "OK"
    return 2, "Replication is down for {} sec".format(timeout)


def process_replication_down(path, timeout):
    if os.path.exists(path):
        status, msg = process_state_file(path, timeout)
        die(status, msg)
    replication_state.set()
    die()


def process_replication_up(lag, warn, crit):
    replication_state.clear()
    if lag < warn:
        die()
    elif lag < crit:
        die(1, '{lag} seconds'.format(lag=lag))
    else:
        die(2, '{lag} seconds'.format(lag=lag))


def check_replication(conn, warn, crit, path, timeout):
    """
    Check replication lag against warn and crit thresholds
    """
    info = conn.info(section='replication')
    role = info.get('role', 'slave')
    if role != 'slave':
        replication_state.clear()
        die()
    lag = info.get('master_last_io_seconds_ago', 0)
    if lag == -1:
        process_replication_down(path, timeout)
    else:
        process_replication_up(lag, warn, crit)

    die()


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-w',
        '--warn',
        type=int,
        default=10,
        help='Warning threshold in seconds')
    parser.add_argument(
        '-c',
        '--crit',
        type=int,
        default=30,
        help='Critical threshold in seconds')
    parser.add_argument(
        '-p',
        '--port',
        type=int,
        default=6379,
        help='Port to connect to')
    parser.add_argument(
        '-t',
        '--timeout',
        type=int,
        default=3600,
        help='Time to fix replication troubles automatically')
    parser.add_argument(
        '-a',
        '--path',
        type=str,
        default=STATE_FILE,
        help='State file created by monrun check replication_lag itself for replication troubles detection')
    args = parser.parse_args()
    with open(os.path.expanduser('~/.redispass')) as redispass:
        password = json.load(redispass)['password']
    for address in ['127.0.0.1', '::1']:
        try:
            conn = StrictRedis(
                host=address, port=args.port, db=0, password=password)
            if conn.ping():
                return check_replication(conn, args.warn, args.crit, args.path, args.timeout)
        except Exception as e:
            if str(e) == 'max number of clients reached':
                die(1, 'conns overlimit')
    die(1, 'redis is dead')


if __name__ == '__main__':
    _main()
