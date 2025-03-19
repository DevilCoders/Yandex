#!/usr/bin/env python

import socket
import time
import sys
import psycopg2
import os

hostname = socket.gethostname()
metrics = ['active', 'idle']

user = 'monitor'
password = ''

conn = psycopg2.connect('dbname=postgres ' +
                        'user=%s password=%s ' % (user, password) +
                        'connect_timeout=1')
cur = conn.cursor()

for metric in metrics:
    cur.execute("select count(*) from " +
                "pg_stat_activity where state='%s';" % metric)
    result = cur.fetchone()[0]
    print("%s %d" % (metric, result))

