#!/usr/bin/env python

import psycopg2
import os
import argparse
import time
import json

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn',
                    type=int,
                    default=1,
                    help='Warning limit')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=0,
                    help='Critical limit')

parser.add_argument('-d', '--delta',
                    type=int,
                    default=3600,
                    help='Critical limit')

args = parser.parse_args()
DELTA_LIMIT = args.delta
STATE_FILE = '/tmp/pg_replication_alive.state'
TS_FORMAT = "%d/%m/%y %H:%M"


def die(code=0, comment="OK"):
    print('{code};{comment}'.format(code=code, comment=comment))


def check_repl_mon_enable(cur):
    cur.execute("SELECT table_name FROM information_schema.tables "
                "WHERE table_schema = 'public' AND table_name = 'repl_mon_settings'")
    if cur.fetchone():
        cur.execute("SELECT value FROM repl_mon_settings WHERE key = 'interval'")
        repl_mon_interval = cur.fetchone()
        if repl_mon_interval and int(repl_mon_interval[0]) == 0:
            return False
    return True


if os.path.exists(STATE_FILE):
    with open(STATE_FILE, 'r') as fd:
        try:
            prev_state = json.load(fd)
        except ValueError:
            prev_state = {}
else:
    with open(STATE_FILE, 'w') as fd:
        fd.write('{}')
    prev_state = {}

try:
    die_code = 0
    die_msg = ""
    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()
    cur.execute("SELECT slot_name FROM pg_replication_slots "
                "    WHERE active = false AND xmin IS NOT NULL and restart_lsn IS NOT NULL;")

    unused_slots = {}
    for row in cur.fetchall():
        unused_slots[row[0]] = ''

    if len(unused_slots) > 0:
        die_code = 1
        die_msg += "There are %d unused replication slots." % len(unused_slots)

    cur.execute("SELECT pg_is_in_recovery()")

    if cur.fetchone()[0] is False and check_repl_mon_enable(cur):
        cur.execute("select replics, " +
                    "extract(epoch from (current_timestamp - ts)) " +
                    "as lag_seconds from repl_mon")
        active_slaves, delta = cur.fetchone()
        die_msg += " There are %d active slave(s). " % active_slaves

        if delta > 30:
            die_code = 1
            die_msg += " repl_mon last update was %d seconds ago." % int(delta)

        if active_slaves <= args.crit:
            die_code = 2
        elif active_slaves <= args.warn:
            die_code = 1

    cur_state = {}
    for slot, ts in unused_slots.iteritems():
        cur_timestamp = int(time.time())
        prev_timestamp = prev_state.get(slot, cur_timestamp)
        time_delta = cur_timestamp - int(prev_timestamp)
        cur_state[slot] = cur_timestamp - time_delta
        if time_delta > DELTA_LIMIT:
            die_code = 2
            fail_timestamp = time.localtime(float(prev_timestamp))
            die_msg += 'Replication slot [ %s ] is inactive since %s. ' \
                       % (slot, time.strftime(TS_FORMAT, fail_timestamp))
    with open(STATE_FILE, 'w') as fd:
        json.dump(cur_state, fd)

    die(die_code, die_msg)
except Exception:
    die(1, "Could not get replication info")
finally:
    try:
        cur.close()
        conn.close()
    except Exception:
        pass
