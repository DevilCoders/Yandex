#!/usr/bin/env python

import argparse
import os
import subprocess
import sys
import time
from datetime import datetime

import psycopg2

RECENT_CHECKPOINT_TIME_FILE = '/tmp/.recent_checkpoint'


def get_pg_version():
    clusters = subprocess.check_output(['pg_lsclusters'], stderr=subprocess.PIPE)
    for line in clusters.splitlines():
        if line.startswith('Ver'):
            continue
        return line.split()[0]
    sys.exit(1)


def get_checkpoint_timeout(args):
    checkpoint_timeout = args.checkpoint_timeout
    if type(checkpoint_timeout) == int:
        return checkpoint_timeout
    elif checkpoint_timeout.endswith('ms'):
        return int(checkpoint_timeout[:-2]) / 1000
    elif checkpoint_timeout.endswith('s'):
        return int(checkpoint_timeout[:-1])
    elif checkpoint_timeout.endswith('min'):
        return int(checkpoint_timeout[:-3]) * 60
    elif checkpoint_timeout.endswith('h'):
        return int(checkpoint_timeout[:-1]) * 60 * 60
    elif checkpoint_timeout.endswith('d'):
        return int(checkpoint_timeout[:-1]) * 60 * 60 * 24
    else:
        return 5 * 60  # default


def get_latest_checkpoint(args):
    controldata = os.path.join('/usr/lib/postgresql', args.version, 'bin/pg_controldata')
    data_dir = os.path.join('/var/lib/postgresql', args.version, 'data')
    try:
        out = subprocess.check_output([controldata, '-D', data_dir], stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as err:
        if args.verbose:
            print(time.strftime('%H:%M:%S ') + str(err).rstrip())
        return None

    for c in out.splitlines():
        if c.startswith('Time of latest checkpoint'):
            dt = datetime.strptime(c[26:].strip(), '%a %d %b %Y %I:%M:%S %p %Z')
            return dt.strftime('%Y-%m-%d %H:%M%S')


def is_waiting_consistency(args):
    current_latest_checkpoint = get_latest_checkpoint(args)
    if not current_latest_checkpoint:
        return False
    if args.verbose:
        print(time.strftime('%H:%M:%S') + ' latest checkpoint: ' + current_latest_checkpoint)

    if os.path.exists(RECENT_CHECKPOINT_TIME_FILE):
        with open(RECENT_CHECKPOINT_TIME_FILE, 'r') as f:
            recent_latest_checkpoint = f.readline()

        checkpoint_timeout = get_checkpoint_timeout(args)
        if args.verbose:
            print(time.strftime('%H:%M:%S') + ' recent checkpoint: ' + recent_latest_checkpoint)
        if recent_latest_checkpoint == current_latest_checkpoint:
            recent_latest_checkpoint_time = os.path.getmtime(RECENT_CHECKPOINT_TIME_FILE)
            return time.time() - recent_latest_checkpoint_time < checkpoint_timeout

    with open(RECENT_CHECKPOINT_TIME_FILE + '.tmp', 'w') as f:
        f.write(current_latest_checkpoint)
    os.rename(RECENT_CHECKPOINT_TIME_FILE + '.tmp', RECENT_CHECKPOINT_TIME_FILE)
    return True


def pg_is_ok(dsn, args):
    try:
        conn = psycopg2.connect(dsn)
        cur = conn.cursor()
        cur.execute('SELECT 42;')
        if cur.fetchone()[0] == 42:
            return True
    except Exception as err:
        if args.verbose:
            print(time.strftime('%H:%M:%S ') + str(err).rstrip())
    finally:
        try:
            cur.close()
            conn.close()
        except Exception:
            pass
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--port', type=int, default=5432, help='Postgres port')
    parser.add_argument('-w', '--wait', type=int, default=60, help='Time to wait before checking checkpoints')
    parser.add_argument('-c', type=str, default='5min', dest='checkpoint_timeout', help='Setting checkpoint_timeout')
    parser.add_argument('-m', '--version', type=str, default=get_pg_version(), help='Major pg version')
    parser.add_argument('-v', '--verbose', action="store_true", help='Some logging')
    args = parser.parse_args()

    start = time.time()

    dsn = 'dbname=postgres connect_timeout=1 port=%d' % args.port

    while time.time() < start + args.wait:
        if pg_is_ok(dsn, args):
            sys.exit(0)
        else:
            time.sleep(1)
    if args.verbose:
        print('Timeout (%d seconds) expired.' % args.wait)

    if is_waiting_consistency(args):
        if args.verbose:
            print(time.strftime('%H:%M:%S ') + 'wait consistency')
        while is_waiting_consistency(args):
            if pg_is_ok(dsn, args):
                sys.exit(0)
            else:
                time.sleep(1)
        if args.verbose:
            print('Checkpoint timeout (%s) expired.' % args.checkpoint_timeout)

    sys.exit(1)


if __name__ == '__main__':
    main()
