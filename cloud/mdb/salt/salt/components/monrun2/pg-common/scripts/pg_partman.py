#!/usr/bin/env python

from __future__ import absolute_import, print_function, unicode_literals

from distutils.version import StrictVersion

import argparse
import sys

import psycopg2

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=int,
                    default=4,
                    help='Warning limit')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=3,
                    help='Critical limit')

args = parser.parse_args()

die_code = 0
die_msg = ''
min_version = StrictVersion('2.0.0')


def die(code=0, comment="OK"):
    if code == 0:
        print("0;OK")
    else:
        print('%d;%s' % (code, comment))
    sys.exit(0)

try:
    die_code = 0

    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()

    cur.execute("SELECT pg_is_in_recovery()")

    if cur.fetchone()[0] is True:
        die(0, "OK")

    cur.execute("select default_version from pg_available_extensions " +
                "where name = 'pg_partman';")
    res = cur.fetchall()
    if not res:
        die(0, "OK")
    max_version = StrictVersion(res[0][0])

    cur.execute("SELECT datname FROM pg_database WHERE "
                "datistemplate = false AND datname != 'postgres';")
    dbnames = cur.fetchall()
    dbnames = [i[0] for i in dbnames]
    not_supported_dbs = []

    for dbname in dbnames:
        dbname_conn = psycopg2.connect('dbname=%s user=monitor '
                                       'connect_timeout=1 host=localhost' % dbname)
        dbname_cur = dbname_conn.cursor()
        dbname_cur.execute("SELECT extversion, extnamespace FROM pg_catalog.pg_extension "
                           "WHERE extname = 'pg_partman';")
        res = dbname_cur.fetchall()
        if not res:
            continue

        version = StrictVersion(res[0][0])
        namespace_oid = str(res[0][1])
        dbname_cur.execute("SELECT nspname FROM pg_catalog.pg_namespace "
                           "WHERE oid = {}".format(namespace_oid))
        namespace = dbname_cur.fetchall()[0][0]

        if version > max_version or version < min_version:
            not_supported_dbs.append(dbname)
            continue

        dbname_cur.execute("SELECT premade, tablename "
                           "FROM {}.check_partitions();".format(namespace))
        res = dbname_cur.fetchall()
        min_premade = None
        min_tablename = None
        for i in res:
            if i[0] < min_premade or min_premade is None:
                min_premade = i[0]
                min_tablename = i[1]

        if min_premade is None:
            die_code = max(die_code, 0)
        elif min_premade < args.crit:
            die_code = 2
            die_msg += "Table %s has only %d premade partitions in %s. " % \
                       (min_tablename, min_premade, dbname)
        elif min_premade < args.warn:
            die_code = max(die_code, 1)
            die_msg += "Table %s has only %d premade partitions in %s. " % \
                       (min_tablename, min_premade, dbname)
        else:
            die_code = max(die_code, 0)

    if not_supported_dbs:
        die_code = max(die_code, 1)
        die_msg += "Not supported version of pg_partman in " + \
                   "%s." % ', '.join(not_supported_dbs)

    die(die_code, die_msg)

except Exception:
    die(1, "Could not get info about premade partitions.")
