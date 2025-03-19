#!/usr/bin/env python
"""
Check for stale transfers
"""

import argparse

import psycopg2


def get_stale(time_limit):
    conn = psycopg2.connect('dbname=dbm user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()

    cur.execute('SELECT id FROM mdb.transfers WHERE started < now() - %(limit)s::interval',
                {'limit': '{limit} seconds'.format(limit=time_limit)})

    return [x[0] for x in cur.fetchall()]


def _main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-l', '--limit',
                        type=int,
                        default=48 * 3600,
                        help='Time limit (seconds)')

    args = parser.parse_args()

    try:
        stale = get_stale(args.limit)
        if stale:
            print('1;{num} transfers are stale: {ids}'.format(num=len(stale), ids=', '.join(stale)))
        else:
            print('0;OK')
    except Exception as exc:
        print('1;Unable to get stale transfers info: {error}'.format(error=repr(exc)))


if __name__ == '__main__':
    _main()
