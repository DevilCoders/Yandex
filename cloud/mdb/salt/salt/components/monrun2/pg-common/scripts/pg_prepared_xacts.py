#!/usr/bin/env python

import argparse
import sys

import psycopg2
from psycopg2.extras import NamedTupleCursor

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=str,
                    default="10m",
                    help='Warning limit')

parser.add_argument('-c', '--crit',
                    type=str,
                    default="30m",
                    help='Critical limit')

parser.add_argument('-a', '--all',
                    type=bool,
                    default=False,
                    help='Check long running transactions in pg_stat_activity')

args = parser.parse_args()

def die(code=0, comment="OK"):
    if code == 0:
        print("0;OK")
    else:
        print('%d;%s' % (code, comment))
    sys.exit(0)

try:
    die_code = 0
    die_msg = ""

    conn = psycopg2.connect("dbname=postgres user=monitor connect_timeout=1 host=localhost")
    cur = conn.cursor(cursor_factory=NamedTupleCursor)

    cur.execute("SELECT pg_is_in_recovery()")
    if cur.fetchone()[0] is True:
        die(0, "OK")

    cur.execute("""
        SELECT *, prepared < current_timestamp - %(crit_ttl)s::interval AS crit,
            date_trunc('second', current_timestamp - prepared) AS age
            FROM pg_prepared_xacts
            WHERE prepared < current_timestamp - least(%(warn_ttl)s::interval, %(crit_ttl)s::interval)
        ORDER BY prepared
    """, {'crit_ttl': args.crit, 'warn_ttl': args.warn})

    for xact in cur.fetchall():
        die_code = max(die_code, 2 if xact.crit else 1)
        die_msg += "Prepared transaction '%s' in database \"%s\" of owner \"%s\" has age %s. " % \
                        (xact.gid, xact.database, xact.owner, xact.age)
    if args.all:
        cur.execute("""
            SELECT *, coalesce(xact_start, query_start) < current_timestamp - %(crit_ttl)s::interval AS crit,
                date_trunc('second', current_timestamp - coalesce(xact_start, query_start)) AS age
                FROM pg_stat_activity
                WHERE coalesce(xact_start, query_start) < current_timestamp - least(%(warn_ttl)s::interval, %(crit_ttl)s::interval)
                AND state != 'idle' AND backend_type = 'client_backend'
            ORDER BY age
        """, {'crit_ttl': args.crit, 'warn_ttl': args.warn})

        for pgsa in cur.fetchall():
            die_code = max(die_code, 2 if pgsa.crit else 1)
            die_msg += "Transaction with pid '%s' in database \"%s\" of user \"%s\" has age %s. " % \
                            (pgsa.pid, pgsa.datname, pgsa.usename, pgsa.age)

    die(die_code, die_msg)

except Exception as e:
    die(1, "Could not get info about queue: %s" % str(e).replace('\n', ''))
