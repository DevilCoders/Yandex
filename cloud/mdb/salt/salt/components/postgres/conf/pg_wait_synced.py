#!/usr/bin/env python
"""
Wait for local postgresql to sync with master
"""

import argparse
import os
import sys
import time

import psycopg2


def get_apply_horizon():
    """
    Get min apply delay
    """
    try:
        with psycopg2.connect('dbname=postgres user=postgres connect_timeout=1 host=localhost') as conn:
            cur = conn.cursor()
            cur.execute('SHOW data_directory')
            base_dir = cur.fetchone()[0]
            for path in [os.path.join(base_dir, 'recovery.conf'), os.path.join(base_dir, 'conf.d/recovery.conf')]:
                if os.path.exists(path):
                    with open(path) as recovery_conf:
                        for line in recovery_conf:
                            if line.startswith('recovery_min_apply_delay = '):
                                return int(line.split()[2].split('m')[0].split("'")[1]) // 1000
    except Exception as exc:
        print(repr(exc))

    return 0


def check_repl_mon_enable(cur):
    cur.execute("SELECT table_name FROM information_schema.tables "
                "WHERE table_schema = 'public' AND table_name = 'repl_mon_settings'")
    if cur.fetchone():
        cur.execute("SELECT value FROM repl_mon_settings WHERE key = 'interval'")
        repl_mon_interval = cur.fetchone()
        if repl_mon_interval and int(repl_mon_interval[0]) == 0:
            return False
    return True


def check_lag(horizon):
    """
    Check that postgresql replication lag is less than horizon + 10 seconds
    """
    try:
        with psycopg2.connect('dbname=postgres user=postgres connect_timeout=1 host=localhost') as conn:
            cur = conn.cursor()

            cur.execute('SELECT pg_is_in_recovery()')
            if cur.fetchone()[0] is False:
                return True

            if check_repl_mon_enable(cur):
                cur.execute("SELECT EXTRACT(EPOCH FROM (current_timestamp - ts)) AS lag_seconds FROM repl_mon")
                lag = cur.fetchone()[0]

                print(lag, horizon)
                return lag < 10 + horizon

            cur.execute("SELECT status FROM pg_stat_wal_receiver")
            status = cur.fetchone()[0]
            print(status)
            return status == 'streaming'

    except Exception:
        return False


def _main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--wait', type=int, default=60, help='Time to wait')
    args = parser.parse_args()
    start = time.time()
    horizon = get_apply_horizon()

    while time.time() < start + args.wait:
        if check_lag(horizon):
            return
        time.sleep(1)

    sys.exit(1)


if __name__ == '__main__':
    _main()
