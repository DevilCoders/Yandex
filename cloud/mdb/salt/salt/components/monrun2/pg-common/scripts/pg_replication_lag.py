#!/usr/bin/env python
import sys
import psycopg2
import argparse

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=int,
                    default=10,
                    help='Warning time limit (seconds)')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=30,
                    help='Critical time limit (seconds)')

args = parser.parse_args()


def die(code=0, comment='OK'):
    print('{code};{comment}'.format(code=code, comment=comment))
    sys.exit(0)


def check_repl_mon_enable(cur):
    cur.execute("SELECT table_name FROM information_schema.tables "
                "WHERE table_schema = 'public' AND table_name = 'repl_mon_settings'")
    if cur.fetchone():
        cur.execute("SELECT value FROM repl_mon_settings WHERE key = 'interval'")
        repl_mon_interval = cur.fetchone()
        if repl_mon_interval and int(repl_mon_interval[0]) == 0:
            return False
    return True


try:
    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()

    cur.execute('SELECT pg_is_in_recovery()')
    if cur.fetchone()[0] is False:
        die()

    if not check_repl_mon_enable(cur):
        die(1, 'Could not get replication lag - repl_mon disabled')

    cur.execute("SELECT EXTRACT(EPOCH FROM (CURRENT_TIMESTAMP - ts)) AS lag_seconds FROM repl_mon")
    time_lag = int(cur.fetchone()[0])

    if time_lag >= args.crit:
        die_code = 2
    elif time_lag >= args.warn:
        die_code = 1
    else:
        die_code = 0

    die(die_code, '{} seconds'.format(time_lag))

except Exception:
    die(1, 'Could not get replication lag')

finally:
    try:
        cur.close()
        conn.close()
    except Exception:
        pass
