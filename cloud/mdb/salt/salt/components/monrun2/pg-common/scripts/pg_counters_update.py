#!/usr/bin/env python

import psycopg2
import sys
import os
import argparse

COUNTERS_ERRORS_FILEPATH = '/var/log/s3/meta_critical_errors.log'


parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=int,
                    default=300,
                    help='Warning time limit (seconds)')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=1200,
                    help='Critical time limit (seconds)')

parser.add_argument('-d', '--dbname',
                    type=str,
                    default="postgres",
                    help='Database to check')

args = parser.parse_args()

def die(code=0, comment="OK"):
    if code == 0:
        print '0;OK'
    else:
        print('%d;%s' % (code, comment))
    sys.exit(0)

try:
    # check critical errors in counters
    if os.path.exists(COUNTERS_ERRORS_FILEPATH) and os.stat(COUNTERS_ERRORS_FILEPATH).st_size != 0:
        die(1, 'There is something in %s' % COUNTERS_ERRORS_FILEPATH)

    conn = psycopg2.connect('dbname=%s user=monitor connect_timeout=1 host=localhost' % args.dbname)
    cur = conn.cursor()

    cur.execute("SELECT pg_is_in_recovery()")

    if cur.fetchone()[0] is True:
        die(0, "OK")
    else:
        cur.execute("select extract(epoch from (current_timestamp - min(last_counters_updated_ts))) from s3.parts")
        time_lag = cur.fetchone()[0]
        if time_lag >= args.crit:
            die_code = 2
        elif time_lag >= args.warn:
            die_code = 1
        else:
            die_code = 0
        die(die_code, '%d seconds' % int(time_lag or 0))
except Exception:
    die(1, "Could not get counters update lag on meta")
finally:
    try:
        cur.close()
        conn.close()
    except Exception:
        pass

