#!/usr/bin/env python

import psycopg2
import sys
import os
import argparse

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=float,
                    default=0.2,
                    help='Warning limit in percent')

parser.add_argument('-c', '--crit',
                    type=float,
                    default=0.05,
                    help='Critical limit')

args = parser.parse_args()

def die(code=0, comment=""):
    if code == 0:
        print "0;OK"
    else:
        print('%d;%s' % (code, comment))

try:
    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()
    cur.execute("SELECT setting FROM pg_settings"
                " WHERE name = 'max_connections';")
    max_connections = int(cur.fetchone()[0])
    cur.execute("SELECT count(*) FROM pg_stat_activity;")
    real_connections = int(cur.fetchone()[0])
    available_connections = max_connections - real_connections
    if available_connections <= args.crit * max_connections:
        die_code = 2
    elif available_connections <= args.warn * max_connections:
        die_code = 1
    else:
        die_code = 0
    die(die_code, "%d available connections" % available_connections)
except Exception as err:
    die(1, "Could not get available connections")
finally:
    try:
        cur.close()
        conn.close()
    except Exception:
        pass
