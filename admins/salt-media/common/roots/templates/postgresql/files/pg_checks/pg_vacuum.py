#!/usr/bin/env python

import psycopg2
import sys
import os
import argparse

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=int,
                    default=7200,
                    help='Warning limit')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=int(1e9),
                    help='Critical limit')

parser.add_argument('-W', '--xid_warn',
                    type=int,
                    default=100e6,
                    help='Warning limit for xids left')

parser.add_argument('-C', '--xid_crit',
                    type=int,
                    default=int(20e6),
                    help='Critical limit for xids left')

args = parser.parse_args()

me = '.'.join(os.path.basename(sys.argv[0]).split('.')[:-1])

def die(code=0, comment="OK"):
    if code == 0:
        print '0;OK'
    else:
        print('%d;%s' % (code, comment))

try:
    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1')
    cur = conn.cursor()

    cur.execute("show transaction_read_only;")

    if 'on' in str(cur.fetchone()[0]):
        die(0, "OK")
    else:
        die_code = 0
        message = ''

        curState = {}
        prevState = {}
        prevPath = os.path.expanduser('/tmp/' + me + '.prev')
        if os.path.exists(prevPath):
            with open(prevPath, 'r') as f:
                try:
                    for line in f.readlines():
                        tokens = line.rstrip().split(';')
                        prevState[tokens[0]] = {'start': int(tokens[1]),
                                                'last': int(tokens[2]),
                                                'req': tokens[3] == 't'}
                except Exception:
                    prev_state = {}

        cur.execute("SELECT datname FROM pg_database WHERE " +
                    "datistemplate = false AND datname != 'postgres';")
        dbnames = cur.fetchall()

        cur.execute("SELECT datname, 2147483647-age(datfrozenxid) as " +
                    "left FROM pg_database;")
        xidsData = cur.fetchall()

        for i in xidsData:
            if i[0] in dbnames:
                if i[1] <= args.xid_crit:
                    die_code = 2
                    message += 'wraparound: ' + i[0] + \
                        'left ' + str(i[1]) + ' xids '
                if i[1] <= args.xid_warn:
                    if die_code == 0:
                        die_code = 1
                    message += 'wraparound: ' + i[0] + \
                        'left ' + str(i[1]) + ' xids '

        maxTime = 0

        for dbname in dbnames:
            conn = psycopg2.connect('user=monitor connect_timeout=1 ' +
                                    'dbname=%s' % dbname[0])
            cur = conn.cursor()
            cur.execute("select current_timestamp;")
            now = int(cur.fetchone()[0].strftime("%s"))
            cur.execute("select psut.relname, " +
                        "psut.last_vacuum, psut.last_autovacuum, " +
                        "case when cast(current_setting('" +
                        "autovacuum_vacuum_threshold') as bigint) " +
                        "+ (cast(current_setting('" +
                        "autovacuum_vacuum_scale_factor') as numeric) " +
                        "* pg_class.reltuples) < psut.n_dead_tup " +
                        "and psut.schemaname not like 'pg_temp%' " +
                        "then true else false end as expect " +
                        "from pg_stat_user_tables psut " +
                        "join pg_class on psut.relid = pg_class.oid;")
            result = cur.fetchall()
            for i in result:
                name = dbname[0] + '.' + i[0]
                curState[name] = {'req': i[3]}
                if name in prevState:
                    if i[3] and not prevState[name]['req']:
                        curState[name]['start'] = now
                    else:
                        curState[name]['start'] = prevState[name]['start']
                else:
                    curState[name]['start'] = now
                avTs = [t for t in i[1:3] if t is not None]
                if avTs:
                    curState[name]['last'] = int(max(avTs).strftime("%s"))
                else:
                    curState[name]['last'] = 0
            cur.close()
            conn.close()
        for i in curState:
            if curState[i]['req']:
                nvTime = now - max(curState[i]['start'], curState[i]['last'])
                if nvTime >= args.warn:
                    message += i + ' '
                    if nvTime > maxTime:
                        maxTime = nvTime
        if maxTime >= args.crit:
            die_code = 2
        elif maxTime >= args.warn:
            die_code = 1
        else:
            message = 'OK'

        die(die_code, message)

        with open(prevPath, 'w') as f:
            for i in curState:
                f.write(i + ';' + str(curState[i]['start']) + ';' +
                        str(curState[i]['last']) + ';' +
                        str(curState[i]['req']) + '\n')
except Exception:
    die(1, "Could not get info about stale vacuumed relations")
