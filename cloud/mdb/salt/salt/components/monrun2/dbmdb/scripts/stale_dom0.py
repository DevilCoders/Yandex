#!/usr/bin/env python
"""
Check for stale dom0 hosts
"""

import argparse

import psycopg2


def get_stale(open_limit, closed_limit):
    """
    Get hosts with too old last heartbeat
    """
    conn = psycopg2.connect('dbname=dbm user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()

    cur.execute(
        """
        SELECT fqdn FROM mdb.dom0_hosts
        WHERE heartbeat IS NULL
              OR heartbeat < now() - %(limit)s::interval
              AND allow_new_hosts = true
        ORDER BY heartbeat
        """,
        {'limit': '{limit} days'.format(limit=open_limit)})

    stale_open = [x[0] for x in cur.fetchall()]

    cur.execute(
        """
        SELECT fqdn FROM mdb.dom0_hosts
        WHERE heartbeat IS NULL
              OR heartbeat < now() - %(limit)s::interval
              AND allow_new_hosts = false
        ORDER BY heartbeat
        """,
        {'limit': '{limit} days'.format(limit=closed_limit)})

    stale_closed = [x[0] for x in cur.fetchall()]

    return stale_open, stale_closed


def _main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-o', '--open',
                        type=int,
                        default=7,
                        help='Open limit (days)')
    parser.add_argument('-c', '--closed',
                        type=int,
                        default=30,
                        help='Closed limit (days)')

    args = parser.parse_args()

    try:
        stale_open, stale_closed = get_stale(args.open, args.closed)
        msg = []
        if stale_open:
            msg.append('{num} open hosts are stale. Top 3: {top3}'.format(
                num=len(stale_open), top3=', '.join(stale_open[:3])))
        if stale_closed:
            msg.append('{num} closed hosts are stale. Top 3: {top3}'.format(
                num=len(stale_closed), top3=', '.join(stale_closed[:3])))
        if msg:
            print('1;{msg}'.format(msg=', '.join(msg)))
        else:
            print('0;OK')
    except Exception as exc:
        print('1;Unable to get stale dom0 hosts info: {error}'.format(error=repr(exc)))


if __name__ == '__main__':
    _main()
