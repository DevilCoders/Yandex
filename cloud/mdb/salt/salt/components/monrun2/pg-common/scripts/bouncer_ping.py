#!/usr/bin/python

import argparse
import socket
import sys
import time

import psycopg2

parser = argparse.ArgumentParser()

parser.add_argument('-n', '--num',
                    type=int,
                    default=3,
                    help='Num retries')

parser.add_argument('-s', '--skip_tls',
                    action='store_true',
                    help='Do not check cert')

args = parser.parse_args()


def die(code=0, comment='OK'):
    print('%d;%s' % (code, comment))

for i in range(args.num):
    try:
        conn_str = 'dbname=postgres user=monitor connect_timeout=1 port=6432'
        if args.skip_tls:
            conn_str += ' host=localhost'
        else:
            conn_str += ' host=%s sslmode=verify-full' % socket.getfqdn()
        conn = psycopg2.connect(conn_str)
        conn.autocommit = True
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

die(2, 'bouncer is dead')
