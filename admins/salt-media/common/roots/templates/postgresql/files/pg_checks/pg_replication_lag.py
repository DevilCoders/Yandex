#!/usr/bin/env python

import psycopg2
import sys
import os
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

def die(code=0, comment="OK"):
    if code == 0:
        print '0;OK'
    else:
        print('%d;%s' % (code, comment))

try:
    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1')
    cur = conn.cursor()

    cur.execute("show transaction_read_only;")

    if 'off' in str(cur.fetchone()[0]):
        die(0, "OK")
    else:
        cur.execute("select extract(epoch from (current_timestamp - ts))" +
                    " as lag_seconds from repl_mon")
        time_lag = cur.fetchone()[0]
        if time_lag >= args.crit:
            die_code = 2
        elif time_lag >= args.warn:
            die_code = 1
        else:
            die_code = 0
        die(die_code, '%d seconds' % int(time_lag))
except Exception:
    die(1, "Could not get replication lag")
finally:
    try:
        cur.close()
        conn.close()
    except Exception:
        pass
