#!/usr/bin/env python

import psycopg2
import sys
import time
import os
import os.path
import argparse

parser = argparse.ArgumentParser()

parser.add_argument('-W', '--xid_warn', type=int, default=100e6, help='Warning limit for xids left')

parser.add_argument('-C', '--xid_crit', type=int, default=20e6, help='Critical limit for xids left')

parser.add_argument('-x', '--xmin_check', action='store_true', default=False, help='Check stale xmin')

parser.add_argument('-w', '--xmin_warn', type=int, default=3600, help='Warning limit for xmin stale seconds')

parser.add_argument('-c', '--xmin_crit', type=int, default=86400, help='Critical limit for xmin stale second')

parser.add_argument('-f', '--xmin_state_file', type=str, default='/tmp/.pg_vacuum_xmin_state', help='File to store xmin state')

args = parser.parse_args()

me = '.'.join(os.path.basename(sys.argv[0]).split('.')[:-1])

XMIN_QUERY = """
    SELECT min(xmin) FROM ( 
        SELECT min(backend_xmin::text::int) AS xmin FROM pg_stat_activity
        UNION ALL
        SELECT min(xmin::text::int) AS xmin FROM pg_replication_slots
    ) xmins
"""

def die(code=0, comment="OK"):
    if code == 0:
        print '0;OK'
    else:
        print('%d;%s' % (code, comment))


class XminState:
    SEPARATOR = ';'

    def __init__(self, xmin, timestamp=None):
        self.xmin = xmin
        self.timestamp = timestamp or int(time.time())

    def __str__(self):
        return '{xmin}{sep}{ts}'.format(xmin=self.xmin, sep=self.SEPARATOR, ts=self.timestamp)

    @classmethod
    def from_str(cls, string):
        try:
            xmin, ts = map(int, string.split(cls.SEPARATOR))
        except Exception:
            raise ValueError('invalid {cls} string representation: "{string}"'.format(cls=cls, string=string))
        return cls(xmin, ts)


def save_xmin_state(xmin, state_file):
    with open(state_file, 'w') as sfile:
       sfile.write(str(xmin))


def get_last_xmin_state(state_file):
    if not os.path.exists(state_file):
        return None
    with open(state_file, 'r') as sfile:
       return XminState.from_str(sfile.read())


def find_stale_xmin_reason(cursor, xmin):
    # Try to find query
    cursor.execute('SELECT usename, datname, pid, state FROM pg_stat_activity WHERE backend_xid = %(xid)s', {'xid': xmin})
    if cursor.rowcount > 0:
        user, dbname, pid, state = cursor.fetchone()
        return 'query pid "{pid}" in database "{dbname}" owner "{owner}" state "{state}"'.format(
            pid=pid, dbname=dbname, owner=user, state=state)

    # Try to find prepared transactions
    cursor.execute('SELECT gid, owner, database FROM pg_prepared_xacts WHERE transaction = %(xid)s', {'xid': xmin})
    if cursor.rowcount > 0:
        gid, owner, dbname = cur.fetchone()
        return 'prepared transaction "{gid}" in database "{dbname}" owner "{owner}"'.format(
            gid=gid, dbname=dbname, owner=owner)

    # Try to find replication slot
    cursor.execute('SELECT slot_name, slot_type FROM pg_replication_slots WHERE xmin = %(xmin)s', {'xmin': xmin})
    if cursor.rowcount > 0:
        slot_name, slot_type = cur.fetchone()
        return '{slot_type} slot "{name}"'.format(slot_type=slot_type, name=slot_name) 

    return 'unknown reason'


def make_xmin_reason_string(xmin, reason):
    return 'stale xmin {xmin} reason: {reason}'.format(xmin=xmin, reason=reason)


try:
    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()

    cur.execute("SELECT pg_is_in_recovery()")

    if cur.fetchone()[0] is True:
        die(0, "OK")
    else:
        die_code = 0
        message = ''

        cur.execute("SELECT datname FROM pg_database WHERE " + "datistemplate = false AND datname != 'postgres';")
        dbnames = cur.fetchall()

        cur.execute("SELECT datname, 2147483647-age(datfrozenxid) as " + "left FROM pg_database;")
        xidsData = cur.fetchall()

        for i in xidsData:
            if i[1] <= args.xid_crit:
                die_code = 2
                message += 'wraparound: ' + i[0] + \
                    ' left ' + str(i[1]) + ' xids '
            elif i[1] <= args.xid_warn:
                if die_code == 0:
                    die_code = 1
                message += 'wraparound: ' + i[0] + \
                    ' left ' + str(i[1]) + ' xids '

        if args.xmin_check:
            cur.execute(XMIN_QUERY)
            current_xmin = XminState(cur.fetchone()[0])
            last_xmin = get_last_xmin_state(args.xmin_state_file)
            if last_xmin is None or current_xmin.xmin > last_xmin.xmin:
                # First run or xmin running, just save state
                save_xmin_state(current_xmin, args.xmin_state_file)
            else:
                # Stale xmin
                if current_xmin.timestamp - last_xmin.timestamp >= args.xmin_crit:
                    die_code = 2
                    message += make_xmin_reason_string(current_xmin.xmin, find_stale_xmin_reason(cur, current_xmin.xmin))
                elif current_xmin.timestamp - last_xmin.timestamp >= args.xmin_warn:
                    die_code = 1
                    message += make_xmin_reason_string(current_xmin.xmin, find_stale_xmin_reason(cur, current_xmin.xmin))

        die(die_code, message)

except Exception:
    die(1, "Could not get info about stale left xids")
