#!/usr/bin/env python

import psycopg2
import sys
import os
import argparse

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=int,
                    default=500,
                    help='Warning limit')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=100,
                    help='Critical limit')

args = parser.parse_args()

def die(code=0, comment=""):
    if code == 0:
        print "0;OK"
    else:
        print('%d;%s' % (code, comment))

try:
    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1')
    cur = conn.cursor()
    cur.execute("select setting from pg_settings " +
                " where name='max_connections';")
    max_connections = int(cur.fetchone()[0])
    cur.execute("select count(*) from pg_stat_activity;")
    real_connections = int(cur.fetchone()[0])
    available_connections = max_connections - real_connections
    if available_connections <= args.crit:
        die_code = 2
    elif available_connections <= args.warn:
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
