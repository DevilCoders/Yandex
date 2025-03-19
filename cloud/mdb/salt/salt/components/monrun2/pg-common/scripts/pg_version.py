#!/usr/bin/env python

import argparse
import re
import sys
from contextlib import closing

import psycopg2

CONNECTION_INFO = 'dbname=postgres user=monitor connect_timeout=1 host=localhost'


def die(code=0, comment='OK'):
    print('{code};{comment}'.format(code=code, comment=comment))
    sys.exit(0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--version', required=True, help='PG version. Example: 10.18')
    parser.add_argument('-e', '--edition', required=True, choices=('default', '1c'), help='PG Edition.')
    parser.add_argument('-p', '--package', required=True, help='PG package full name.')
    args = parser.parse_args()

    try:
        with closing(psycopg2.connect(CONNECTION_INFO)) as conn:
            with closing(conn.cursor()) as cur:
                cur.execute("SELECT version()")
                row_version, = cur.fetchall()[0]
                match = re.search(r'PostgreSQL ([0-9]+\.[0-9]+) \(?[a-zA-Z]+ (.+?)\).+', row_version)
                if not match:
                    die(1, 'Could not parse pg version.')

                version, package = match.groups()
                major = version.split('.')[0]

                if version != args.version:
                    major_exp = args.version.split('.')[0]
                    if major != major_exp:
                        die(2, 'Pg major version mismatched. Got: {}, expected: {}'.format(version, args.version))
                    die(1, 'Pg minor version mismatched. Got: {}, expected: {}'.format(version, args.version))

                if not (package.startswith(version) and args.package.startswith(package)):
                    die(2, 'Pg package mismatched. Got: {}, expected: {}'.format(package, args.package))

                if (args.edition == '1c') != package.startswith('{0}-1c'.format(version)):
                    die(2, 'Pg edition mismatched. Expected: {}'.format(args.edition))

    except Exception as e:
        exp = str(e).encode('string_escape')[:350]
        die(1, '{exc}: {value}'.format(exc=type(e).__name__, value=exp))

    die()


if __name__ == '__main__':
    main()
