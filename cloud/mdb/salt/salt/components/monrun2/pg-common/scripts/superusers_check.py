#!/usr/bin/env python

import argparse
import sys
from contextlib import closing

import psycopg2

CONNECTION_INFO = 'dbname=postgres user=monitor connect_timeout=1 host=localhost'


def die(code=0, comment='OK'):
    print('{code};{comment}'.format(code=code, comment=comment))
    sys.exit(0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-su', '--superusers', nargs='+', default=['admin'], help='Valid Extra Superusers.')
    args = parser.parse_args()

    su = {x for x in args.superusers}
    su.add('postgres')

    try:
        with closing(psycopg2.connect(CONNECTION_INFO)) as conn:
            with closing(conn.cursor()) as cur:
                cur.execute("SELECT rolname FROM pg_roles WHERE rolsuper")
                users = {x[0] for x in cur.fetchall()}

                unwanted_su = users - su
                if unwanted_su:
                    die(2, 'Found unwanted superusers: {su}'.format(su=', '.join(unwanted_su)))

    except Exception:
        die(1, 'Could not get superusers.')

    die()


if __name__ == '__main__':
    main()

