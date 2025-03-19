#!/opt/yandex/dbaas-cron/bin/python
"""
Monitoring for offline billing lag
"""

import argparse
import logging
import sys
import time

from kazoo.client import KazooClient


def die(status=0, message='OK'):
    """
    Report status to juggler
    """
    print(f'{status};{message}')
    sys.exit(0)


def check_lag(zk_hosts, max_lag):
    """
    Get last report timestamp and compare it with current time
    """
    logging.getLogger('kazoo').setLevel(logging.FATAL)
    client = KazooClient(zk_hosts, connection_retry=3, command_retry=3, timeout=1)
    client.start()
    lag = int(time.time() - client.get('/offline_hosts/last_report')[1].mtime // 1000)
    if lag > max_lag:
        die(2, f'{lag} seconds')
    die()


def _main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-z',
        '--hosts',
        type=str,
        required=True,
        help='zk hosts',
    )

    parser.add_argument(
        '-l',
        '--lag',
        type=int,
        required=True,
        help='max report lag in seconds',
    )

    args = parser.parse_args()

    try:
        check_lag(args.hosts, args.lag)
    except Exception as exc:
        die(1, repr(exc))


if __name__ == '__main__':
    _main()
