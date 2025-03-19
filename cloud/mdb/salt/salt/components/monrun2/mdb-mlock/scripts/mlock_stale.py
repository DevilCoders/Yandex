#!/usr/bin/env python3
"""
Pushclient lag/commit delay monitoring
"""
import argparse
import datetime
import json
import subprocess
import sys

from dateutil import parser as dt_parser


def parse_args():
    """
    Parse known cmd args
    """
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '-w',
        '--warn-time',
        type=int,
        default=7 * 24 * 3600,
        help='Warning time limit (seconds)')

    parser.add_argument(
        '-c',
        '--crit-time',
        type=int,
        default=14 * 24 * 3600,
        help='Critical time limit (seconds)')

    return parser.parse_args()


def die(code=0, comment=None):
    """
    Passive-check message formatter
    """
    print('{code};{message}'.format(
        code=code, message=comment if comment else 'OK'))
    sys.exit(0)


def parse_lock(line):
    """
    Parse lock list line
    """
    parsed = json.loads(line)
    parsed['create_ts'] = dt_parser.parse(parsed['create_ts'])
    return parsed


def format_message(warn_stale, crit_stale):
    """
    Format message with stale locks
    """
    code = 0
    message = ''
    if warn_stale:
        code = 1
        message = '{num} lock(s) exceed warn limit: {ids}'.format(
            num=len(warn_stale), ids=', '.join([x['id'] for x in warn_stale]))
    if crit_stale:
        code = 2
        if message:
            message += '; '
        message += '{num} lock(s) exceed crit limit: {ids}'.format(
            num=len(crit_stale), ids=', '.join([x['id'] for x in crit_stale]))

    die(code, message)


def check_for_stale_locks(warn_limit, crit_limit):
    """
    Check for stale locks with mlock cli
    """
    now = datetime.datetime.now(datetime.timezone.utc)
    warn_stale = []
    crit_stale = []

    proc = subprocess.Popen(
        ['/usr/local/bin/mlock', '-c', '/home/monitor/.mlock-cli.yaml', 'list'],
        stderr=subprocess.PIPE,
        stdout=subprocess.PIPE,
    )

    while proc.poll() is None:
        line = proc.stdout.readline()
        if not line:
            continue
        parsed = parse_lock(line)
        delta = (now - parsed['create_ts']).total_seconds()
        if delta > crit_limit:
            crit_stale.append(parsed)
        elif delta > warn_limit:
            warn_stale.append(parsed)

    if proc.poll() != 0:
        die(1, 'mlock list exited with {code}: {stderr}'.format(
            code=proc.poll(), stderr=proc.stderr.read().decode().replace('\n', ' ')))

    for line in proc.stdout.readlines():
        if not line:
            break
        parsed = parse_lock(line)
        delta = (now - parsed['create_ts']).total_seconds()
        if delta > crit_limit:
            crit_stale.append(parsed)
        elif delta > warn_limit:
            warn_stale.append(parsed)

    format_message(warn_stale, crit_stale)


def _main():
    args = parse_args()
    check_for_stale_locks(args.warn_time, args.crit_time)


if __name__ == '__main__':
    _main()
