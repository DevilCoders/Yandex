#!/usr/bin/env python3
"""
Wait for ClickHouse server to start up.
"""

import argparse
import logging
import os
import subprocess
import sys
import time

BASE_TIMEOUT = 600
PART_LOAD_SPEED = 5  # in data parts per second


def main():
    """
    Program entry point.
    """
    args = parse_args()

    if not args.quiet:
        logging.basicConfig(level='INFO', format='%(message)s')

    deadline = time.time() + get_timeout(args)

    ch_is_alive = False
    while time.time() < deadline:
        if is_clickhouse_alive():
            ch_is_alive = True
            break
        time.sleep(1)

    if ch_is_alive:
        warmup_system_users()
        sys.exit(0)

    logging.error('ClickHouse is dead')
    sys.exit(1)


def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-w', '--wait', type=int,
        help='Time to wait, in seconds. If not set, the timeout is determined dynamically based on data part count.')
    parser.add_argument(
        '-q', '--quiet', action='store_true', default=False, help='Quiet mode.')
    parser.add_argument(
        '-p', '--port',
        type=int,
        help='ClickHouse HTTP(S) port to use.')
    parser.add_argument(
        '-s', '--ssl',
        action="store_true",
        help='Use HTTPS rather than HTTP.')
    parser.add_argument(
        '--ca_bundle',
        help='Path to CA bundle to use.')
    return parser.parse_args()


def get_timeout(args):
    """
    Calculate and return timeout.
    """
    if args.wait:
        return args.wait

    return BASE_TIMEOUT + int(get_part_count() / PART_LOAD_SPEED)


def get_part_count():
    """
    Return the number of data parts in ClickHouse data dir.
    """
    output = execute('find -L /var/lib/clickhouse/data/ -mindepth 3 -maxdepth 3 -type d | wc -l')
    return int(output)


def is_clickhouse_alive():
    """
    Check if ClickHouse server is alive or not.
    """
    try:
        os.chdir('/')
        output = execute('timeout 5 sudo -u monitor /usr/bin/ch-monitoring ping')
        if output == '0;OK\n':
            return True

    except Exception as e:
        logging.error('Failed to perform ch_ping check: %s', repr(e))

    return False


def warmup_system_users():
    execute('clickhouse-client --user _admin --query "select count() from system.users"')


def execute(command):
    """
    Execute the specified command, check return code and return its output on success.
    """
    proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    stdout, stderr = proc.communicate()

    if proc.returncode:
        msg = '"{0}" failed with code {1}'.format(command, proc.returncode)
        if stderr:
            msg = '{0}: {1}'.format(msg, stderr.decode())

        raise RuntimeError(msg)

    return stdout.decode()


if __name__ == '__main__':
    main()
