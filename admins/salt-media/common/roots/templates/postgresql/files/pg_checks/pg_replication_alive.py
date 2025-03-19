#!/usr/bin/env python

import psycopg2
import sys
import os
import argparse

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=int,
                    default=1,
                    help='Warning limit')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=0,
                    help='Critical limit')

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
    if 'on' in str(cur.fetchone()[0]):
        die()
    else:
        cur.execute("select replics, " +
                    "extract(epoch from (current_timestamp - ts)) " +
                    "as lag_seconds from repl_mon")
        active_slaves, delta = cur.fetchone()
        die_msg = "%d active slave(s)" % active_slaves
        if active_slaves <= args.crit:
            die_code = 2
        elif active_slaves <= args.warn:
            die_code = 1
        else:
            die_code = 0
        if delta > 30:
            if die_code == 0:
                die_code = 1
            die_msg += " repl_mon last update %d seconds ago" % int(delta)
        die(die_code, die_msg)
except Exception:
    die(1, "Could not get replication info")
finally:
    try:
        cur.close()
        conn.close()
    except Exception:
        pass
