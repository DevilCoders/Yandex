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


parser.add_argument('-w', '--warn-limit',
                    type=int,
                    default=20,
                    help='Warning limit')

parser.add_argument('-c', '--crit-limit',
                    type=int,
                    default=1000,
                    help='Critical limit')

parser.add_argument('-e', '--exclude-users',
                    type=str,
                    default='',
                    help='Exclude users from monitoring')

parser.add_argument('-r', '--exclude-errors',
                    type=str,
                    default='OD_ECLIENT_READ',
                    help='Exclude specific errors from monitoring')

args = parser.parse_args()

exclude_users = args.exclude_users.split(',')
exclude_errors = args.exclude_errors.split(',')

def die(code=0, comment='OK'):
    print('%d;%s' % (code, comment))
    exit(0)

def pretty_print_top_errs(err_map, n):
    fmt = ""
    for elem in sorted([e for e in err_map.items()], key=lambda x : -x[1])[:n]:
        error, count = elem
        if int(count) > 0:
            fmt += "%s: %d " % elem

    return fmt

for i in range(args.num):
    try:

        conn = psycopg2.connect('host=localhost port=6432 dbname=pgbouncer user=monitor connect_timeout=1')

        conn.autocommit = True
        cur = conn.cursor()
        cur.execute('show errors_per_route')

        err_map = {}

        for e in cur.fetchall():
            if e[0] in exclude_errors: continue

            if e[0] not in err_map:
                err_map[e[0]] = 0

            if e[1] in exclude_users: continue

            err_map[e[0]] += e[3] # [('OD_ROUTER_ERROR','user', 'database' 0L),  ....

        msg = pretty_print_top_errs(err_map, 5)

        err_cnt = sum(e[1] for e in err_map.items())

        if err_cnt > args.crit_limit:
            die(2, msg)

        if err_cnt > args.warn_limit:
            die(1, msg)

        cur.close()
        conn.close()
        die(0, "nice")
    except Exception:
        time.sleep(1)

die(1, 'bouncer is dead') # do not raise crit here as we have separate monitoring for this

