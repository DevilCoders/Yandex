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

parser.add_argument('-s', '--socket',
                    action='store_true',
                    help='Connect via socket')

args = parser.parse_args()


def die(code=0, comment='OK'):
    print('%d;%s' % (code, comment))


error = ''
for i in range(args.num):
    try:
        conn_str = 'dbname=postgres user=monitor connect_timeout=1'
        if not args.socket:
            conn_str += ' host=%s sslmode=verify-full' % socket.getfqdn()
        conn = psycopg2.connect(conn_str)
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
    except Exception as exc:
        error = str(exc)
        time.sleep(1)

if 'server certificate' in error:
    die(1, 'Incorrect TLS cert')
else:
    die(2, 'PostgreSQL is dead')
