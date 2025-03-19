#!/usr/bin/env python
"""
Check the groups are unbalanced
"""

import argparse

import psycopg2


def get_unbalanced_groups(varcoef_limit):
    conn = psycopg2.connect('dbname=deploydb user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()

    cur.execute(
        """
        WITH master_stats AS (
            SELECT group_id, master_id, COUNT(*) AS cnt
            FROM deploy.minions
            WHERE NOT deleted AND auto_reassign
            GROUP BY group_id, master_id
        )
        SELECT name, stddev_pop(cnt) / avg(cnt) AS varcoef
        FROM master_stats
        JOIN deploy.groups USING (group_id)
        GROUP BY name
        HAVING stddev_pop(cnt) / avg(cnt) > %(limit)s;
        """,
        {'limit': varcoef_limit})

    return ['{}: {}'.format(x[0], x[1]) for x in cur.fetchall()]


def _main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-l', '--limit',
                        type=float,
                        default=0.33,
                        help='Variation coefficient limit')

    args = parser.parse_args()

    try:
        groups = get_unbalanced_groups(args.limit)
        if len(groups):
            print('2;{num} groups are unbalanced: {groups}'.format(num=len(groups), groups=', '.join(groups)))
        else:
            print('0;OK')
    except Exception as exc:
        print('1;Unable to get unbalanced groups info: {error}'.format(error=repr(exc)))


if __name__ == '__main__':
    _main()
