#!/usr/bin/env python

import socket
import time
import sys
import psycopg2
import os

metrics = ['numbackends', 'xact_commit', 'xact_rollback', 'blks_read',
           'blks_hit', 'tup_returned', 'tup_fetched', 'tup_inserted',
           'tup_updated', 'tup_deleted']

user = 'monitor'
password = ''

conn = psycopg2.connect('dbname=postgres ' +
                        'user=%s password=%s ' % (user, password) +
                        'connect_timeout=1')
cur = conn.cursor()

res = cur.execute("show transaction_read_only")
tr_readonly = cur.fetchone()
if tr_readonly[0] == "off":
    is_master = "master"
    print("is_master %d" % (1))

    cur.execute("select datname, pg_database_size(datname) from pg_catalog.pg_database \
            where datname not in ('template0', 'template1', 'postgres');")
    for result in cur.fetchall():
        print("db.%s.size %d" % (result[0].replace('.', '_'), result[1]))
else:
    is_master = "replica"
    print("is_master %d" % (0))

for metric in metrics:
    cur.execute("SELECT sum(%s) FROM pg_stat_database;" % metric)
    result = cur.fetchone()[0]
    print("%s_%s %d" % (is_master, metric, result))
    print("%s %d" % (metric, result))

