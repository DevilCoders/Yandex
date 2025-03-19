#!/usr/bin/env python

import argparse
import json
import os
import time

import psycopg2

STATE_FILE = '/tmp/pg_replication_alive_dbaas.state'
TS_FORMAT = '%d/%m/%y %H:%M'


def die(code=0, comment='OK'):
    if code == 0:
        comment = 'OK'
    print('{code};{comment}'.format(code=code, comment=comment))


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-d', '--delta', type=int, default=3600, help='Critical limit')

    args = parser.parse_args()
    with open('/etc/dbaas.conf') as dbaas_conf:
        ha_hosts = json.load(dbaas_conf)['cluster_nodes']['ha']
        ha_hosts = [host.replace('.', '_').replace('-', '_') for host in ha_hosts]
    delta_limit = args.delta

    if os.path.exists(STATE_FILE):
        with open(STATE_FILE, 'r') as state:
            try:
                prev_state = json.load(state)
            except ValueError:
                prev_state = {}
    else:
        with open(STATE_FILE, 'w') as state:
            state.write('{}')
        prev_state = {}

    try:
        die_code = 0
        die_msg = ''
        conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')
        cur = conn.cursor()
        cur.execute('SELECT slot_name FROM pg_replication_slots '
                    '    WHERE active = false AND xmin IS NOT NULL and restart_lsn IS NOT NULL;')

        unused_slots = {}
        for row in cur.fetchall():
            unused_slots[row[0]] = ''

        if unused_slots:
            die_code = 1
            die_msg += 'There are {count} unused replication slots.'.format(count=len(unused_slots))

        cur.execute('select pg_is_in_recovery();')
        if not cur.fetchone()[0]:  # it's master
            cur.execute('SELECT application_name FROM pg_stat_replication')
            res = cur.fetchall()
            active_replicas = set([row[0] for row in res])

            if None in active_replicas:
                active_replicas.remove(None)

            die_msg += ' There are {count} hosts in HA. '.format(count=len(ha_hosts))
            die_msg += ' There are {count} active slave(s)'.format(count=len(active_replicas))

            active_ha_replicas = active_replicas & set(ha_hosts)
            die_msg += ' and {count} ha replicas are active '.format(count=len(active_ha_replicas))

            if (len(active_ha_replicas) == 0) and (len(ha_hosts) > 2):  # crit
                die_code = 2
            elif len(active_ha_replicas) < len(ha_hosts) - 1:  # warn
                die_code = 1

        cur_state = {}
        for slot, _ in unused_slots.iteritems():
            cur_timestamp = int(time.time())
            prev_timestamp = prev_state.get(slot, cur_timestamp)
            time_delta = cur_timestamp - int(prev_timestamp)
            cur_state[slot] = cur_timestamp - time_delta
            if time_delta > delta_limit:
                die_code = 2
                fail_timestamp = time.localtime(float(prev_timestamp))
                die_msg += 'Replication slot [ {slot} ] is inactive since {ts}. '\
                    .format(slot=slot, ts=time.strftime(TS_FORMAT, fail_timestamp))
        with open(STATE_FILE, 'w') as state:
            json.dump(cur_state, state)

        die(die_code, die_msg)
    except Exception:
        die(1, 'Could not get replication info')
    finally:
        try:
            cur.close()
            conn.close()
        except Exception:
            pass


if __name__ == '__main__':
    main()
