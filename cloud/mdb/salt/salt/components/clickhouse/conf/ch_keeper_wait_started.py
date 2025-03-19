#!/usr/bin/env python3
"""
Wait for ClickHouse keeper to start up.
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

    is_alive = False
    while time.time() < deadline:
        if is_keeper_alive(args.port):
            is_alive = True
            break
        time.sleep(1)

    if is_alive:
        sys.exit(0)

    logging.error('ClickHouse Keeper is dead')
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
        default=2181,
        help='ClickHouse Keeper port to use.')
    return parser.parse_args()


def get_timeout(args):
    """
    Calculate and return timeout.
    """
    if args.wait:
        return args.wait

    return BASE_TIMEOUT


def is_keeper_alive(port):
    """
    Check if ClickHouse server is alive or not.
    """
    try:
        os.chdir('/')
        output = execute('echo ruok | nc localhost {0}'.format(port))
        if output == 'imok':
            return True

    except Exception as e:
        logging.error('Failed to perform keeper check: %s', repr(e))

    return False


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
