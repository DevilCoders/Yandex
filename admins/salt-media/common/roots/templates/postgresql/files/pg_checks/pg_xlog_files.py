#!/usr/bin/env python

import psycopg2
import sys
import os
import json
import datetime
import argparse

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=int,
                    default=300,
                    help='Warning limit')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=900,
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
        die(0, "OK")
    else:
        prev_path = os.path.expanduser('/tmp/pg_xlog.prev')
        prev = {'last': 0, 'fail': 0}
        if os.path.exists(prev_path):
            with open(prev_path, 'r') as f:
                try:
                    prev = json.loads(''.join(f.readlines()))
                except Exception:
                    pass
        cur.execute("select last_archived_time, last_failed_time " +
                    "from pg_stat_archiver;")
        res = cur.fetchone()
        if res[0] is None:
            res = (datetime.datetime(1970, 1, 1, 0), res[1])
        if res[1] is None:
            res = (res[0], datetime.datetime(1970, 1, 1, 0))
        current = {'last': int(res[0].strftime("%s")),
                   'fail': int(res[1].strftime("%s"))}
        if current['last'] == prev['last']:
            if current['fail'] > current['last'] + args.crit:
                die(2, "Archiver stuck at " + res[0].strftime("%F %H:%M:%S"))
            elif current['fail'] > current['last'] + args.warn:
                die(1, "Archiver stuck at " + res[0].strftime("%F %H:%M:%S"))
            else:
                die(0, "Archiver ok")
        else:
            die(0, "Archiver ok")

        with open(prev_path, 'w') as f:
            f.write(json.dumps(current))
except Exception:
    die(1, "Could not get info about not archived xlogs")
finally:
    try:
        cur.close()
        conn.close()
    except Exception:
        pass
