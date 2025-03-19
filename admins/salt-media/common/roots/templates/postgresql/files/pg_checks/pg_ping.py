#!/usr/bin/python

import psycopg2
import sys
import os
import argparse
import time

parser = argparse.ArgumentParser()

parser.add_argument('-n', '--num',
                    type=int,
                    default=3,
                    help='Num retries')

args = parser.parse_args()

def die(code=0, comment='OK'):
    print('%d;%s' % (code, comment))

for i in xrange(args.num):
    try:
        conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1')
        cur = conn.cursor()
        cur.execute('select 1;')
        if cur.fetchone()[0] == 1:
            die()
        else:
            time.sleep(1)
            continue
        cur.close()
        conn.close()
        sys.exit(0)
    except Exception:
        time.sleep(1)

die(2, 'PostgreSQL is dead')
