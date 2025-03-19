#!/usr/bin/env python

import argparse
import contextlib
import pyodbc
import sys

parser = argparse.ArgumentParser()
parser.add_argument('-H', '--host', type=str, help='SQLServer host', required=True)
parser.add_argument('-P', '--port', type=int, default=1433, help='SQLServer port')
parser.add_argument('-u', '--user', type=str, help='SQLServer login to connect', required=True)
parser.add_argument('-p', '--password', type=str, help='SQLServer password to connect', required=True)
parser.add_argument('-d', '--database', type=str, help='SQLServer database to connect')
parser.add_argument('-q', '--query', type=str, default='SELECT 1 + 2', help='Query to check connection')

args = parser.parse_args()

dsn = 'DRIVER={{ODBC Driver 17 for SQL Server}};SERVER=tcp:{host},{port};UID={user};PWD={password}'.format(
    host=args.host,
    port=args.port,
    user=args.user,
    password=args.password,
)
if args.database:
    dsn += ';DATABASE=' + args.database

conn = pyodbc.connect(dsn)
with contextlib.closing(conn) as conn:
    cur = conn.cursor()
    cur.execute(args.query)
    res = cur.fetchone()
    assert res is not None
    assert res[0] is not None
