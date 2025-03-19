#!/usr/bin/env python

import psycopg2
import sys
import os
import argparse
import re

parser = argparse.ArgumentParser()

parser.add_argument('-d', '--databases',
                    type=str,
                    help='Comma-separated list of dbs')

args = parser.parse_args()

def die(code=0, comment=""):
    if code == 0:
        print "0;OK"
    else:
        print('%d;%s' % (code, comment))

conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1')
cur = conn.cursor()
cur.execute("SELECT datname FROM pg_database WHERE datistemplate = false " +
            "AND datname != 'postgres';")

dbs = list(map(lambda x: x[0], cur.fetchall()))

prefixes = {}

level = 0

for i in dbs:
    if i:
        try:
            conn = psycopg2.connect('dbname=%s user=monitor connect_timeout=1' % (i))
            cur = conn.cursor()
            cur.execute("select host_name from plproxy.hosts;")
            host = str(cur.fetchone()[0])

            prefix = re.sub(r'[0-9]{2}[a-z]{1}\.[a-z]*\.yandex\.net', '', host)

            cur.execute("select count(*) from plproxy.parts;")
            total_parts = int(cur.fetchone()[0])

            cur.execute("select count(part_id) from plproxy.priorities where priority=0;")
            parts_rw = int(cur.fetchone()[0])

            if total_parts - parts_rw != 0:
                level = 2

            if prefix in prefixes:
                if prefixes[prefix]['ro'] < total_parts - parts_rw:
                    prefixes[prefix]['ro'] = total_parts - parts_rw
            else:
                prefixes[prefix] = {'ro': total_parts - parts_rw,
                                    'total': total_parts}
        except Exception:
            pass
        finally:
            try:
                cur.close()
                conn.close()
            except Exception:
                pass

if level > 0:
    message = ''
    for i in prefixes:
        if prefixes[i]['ro'] != 0:
            message += i + ': ' + str(prefixes[i]['ro']) + '/' + str(prefixes[i]['total'])
    die(2, message)
else:
    die()
