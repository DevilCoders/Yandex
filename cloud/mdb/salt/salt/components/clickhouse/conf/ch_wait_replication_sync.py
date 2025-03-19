#!/usr/bin/env python3
"""
Wait for ClickHouse server to sync replication with other replicas.
"""

import argparse
import subprocess
import sys
import time
import logging


def main():
    """
    Program entry point.
    """
    args = parse_args()
    if not args.quiet:
        logging.basicConfig(level='INFO', format='%(message)s')

    now = time.time()
    final = now + args.timeout
    while now <= final:
        status = check_lag()
        if status <= args.status:
            sys.exit(0);
        time.sleep(args.pause)
        now = time.time()

    logging.error('ClickHouse can\'t sync replica for {timeout} seconds'.format(timeout=args.timeout))
    sys.exit(1)


def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-s', '--status', type=int, default=0,
        help='Replication lag status, 0 = OK (default), 1 = WARN, 2 = CRIT.')
    parser.add_argument(
        '-t', '--timeout', type=int, default=3*24*60*60,
        help='Maximum wait time in seconds, default 3 days (259200 seconds).')
    parser.add_argument(
        '-p', '--pause', type=int, default=30,
        help='Pause between request in seconds, default 30 seconds.')
    parser.add_argument(
        '-q', '--quiet', action='store_true', default=False,
        help='Quiet mode.')
    return parser.parse_args()


def check_lag():
    """
    Check clickhouse replication lag
    """
    process = subprocess.Popen(['/usr/bin/ch-monitoring', 'replication-lag'], stdout=subprocess.PIPE)
    stdout, _ = process.communicate()
    stdout = stdout.decode("utf-8")
    res = stdout.split(';')
    if res[0] in ['0', '1', '2']:
        return int(res[0])
    logging.error('Can\'t parse ch-monitoring output: {}'.format(stdout))
    return 3


if __name__ == '__main__':
    main()
