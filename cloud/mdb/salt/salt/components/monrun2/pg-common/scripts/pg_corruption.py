#!/usr/bin/env python

import psycopg2
import argparse
from contextlib import closing

parser = argparse.ArgumentParser()

parser.add_argument('-d', '--debug',
                    help='Raise exception if any.',
                    action='store_true')

time_interval = 600
args = parser.parse_args()


def die(code=0, comment="OK"):
    if code == 0:
        print('0;OK')
    else:
        print('%d;%s' % (code, comment))


def get_corruptions(cur):
    cur.execute("select * from pg_log_errors_stats() where message like '%CORRUPTED%'"
                " and time_interval={time_interval} and type = 'ERROR';"
                .format(time_interval=time_interval))
    result = cur.fetchall()
    stat = [(column[2], column[3]) for column in result]

    return ', '.join('{0}({1})'.format(e[0], e[1]) for e in stat)


try:
    with closing(psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')) as conn:
        with closing(conn.cursor()) as cur:
            cur.execute("select pg_is_in_recovery()")
            is_replica = cur.fetchone()[0]
            message = 'OK'
            corruptions = get_corruptions(cur)
            if len(corruptions) > 0:
                die_code = 2
                message = 'Possible data corruption: {0} for last {1} seconds'.format(corruptions, time_interval)
            else:
                die_code = 0

            die(die_code, message)

except Exception as e:
    if args.debug:
        raise(e)
    die(1, "Could not get pg_corruption")
