#!/usr/bin/env python

import argparse
import sys
import os
import psycopg2

COUNTERS_ERRORS_FILEPATH = '/var/log/s3/critical_errors.log'


parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=long,
                    default=10000,
                    help='Warning limit')

parser.add_argument('-c', '--crit',
                    type=long,
                    default=100000,
                    help='Critical limit')

parser.add_argument('--warn-lag',
                    type=long,
                    default=300,
                    dest="warn_lag",
                    help='Warning time queue updating lag (seconds)')

parser.add_argument('--crit-lag',
                    type=long,
                    default=1200,
                    dest="crit_lag",
                    help='Critical time queue updating lag (seconds)')

parser.add_argument('--warn-chunk-size',
                    type=long,
                    default=200000,
                    dest="warn_chunk_size",
                    help='Warning chunks size more than')

parser.add_argument('-d', '--dbname',
                    type=str,
                    default="postgres",
                    help='Database to check')

parser.add_argument('-s', '--schema',
                    type=str,
                    default="public",
                    help='Table schema')

parser.add_argument('-t', '--table',
                    type=str,
                    help='Table name')

args = parser.parse_args()

def die(code=0, comment="OK"):
    if code == 0:
        print("0;OK")
    else:
        print('%d;%s' % (code, comment))
    sys.exit(0)

try:
    if not args.table:
        die(0, "OK")

    # check critical errors in counters
    if os.path.exists(COUNTERS_ERRORS_FILEPATH) and os.stat(COUNTERS_ERRORS_FILEPATH).st_size != 0:
        die(1, 'There is something in %s' % COUNTERS_ERRORS_FILEPATH)

    conn = psycopg2.connect("dbname=%s user=monitor connect_timeout=1 host=localhost" % args.dbname)
    cur = conn.cursor()

    cur.execute("SELECT pg_is_in_recovery()")
    if cur.fetchone()[0] is True:
        die(0, "OK")

    # check queue size
    cur.execute("""
        SELECT n_live_tup FROM pg_stat_user_tables
            WHERE schemaname = %(schema)s AND relname = %(table)s
    """, {'schema': args.schema, 'table': args.table})

    if cur.rowcount < 1:
        die(1, "Couldn't find relation %s.%s" % (args.schema, args.table))
    ntuples = cur.fetchone()[0]
    if ntuples > args.crit:
        die(2, "%s.%s queue has %d rows" % (args.schema, args.table, ntuples))
    elif ntuples > args.warn:
        die(1, "%s.%s queue has %d rows" % (args.schema, args.table, ntuples))


    # check queue minimum created_at
    cur.execute("""
        SELECT extract(epoch from (current_timestamp - created_ts)) FROM s3.chunks_counters_queue ORDER BY id LIMIT 1
    """)

    if cur.rowcount > 0:
        time_lag = cur.fetchone()[0]
        if time_lag >= args.crit_lag:
            die(2, 'Counters queue has not been updated %d seconds' % int(time_lag))
        elif time_lag >= args.warn_lag:
            die(1, 'Counters queue has not been updated %d seconds' % int(time_lag))

    # check negative counters
    cur.execute("""
        SELECT bid FROM s3.chunks_counters WHERE simple_objects_count < 0 OR simple_objects_size < 0
            OR multipart_objects_count < 0 OR multipart_objects_size < 0
            OR objects_parts_count < 0 OR objects_parts_size < 0 LIMIT 1
    """)
    if cur.rowcount > 0:
        die(2, 'Negative counters found!')

    # check chunks size (that splitter is working)
    cur.execute("""
        SELECT bid, cid, start_key, end_key FROM s3.chunks_counters JOIN s3.chunks USING (bid, cid)
        WHERE simple_objects_count + multipart_objects_count > %(threshold)s
    """, {'threshold': args.warn_chunk_size})
    for (bid, cid, start_key, end_key) in list(cur.fetchall()):
        cur.execute("""SELECT count(*) FROM s3.objects WHERE bid = %(bid)s
                        AND (%(start_key)s IS NULL OR name >= %(start_key)s)
                        AND (%(end_key)s IS NULL OR name < %(end_key)s)
        """, {'bid': bid, 'start_key': start_key, 'end_key': end_key})
        if cur.fetchone()[0] > args.warn_chunk_size:
            die(1, 'Chunk (%s, %s) has size > %s' % (bid, cid, args.warn_chunk_size))

    die(0, "OK")

except Exception as e:
    die(1, "Could not get info about queue: %s" % e)
