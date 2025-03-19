#!/usr/bin/env python3

import sys
import argparse
import subprocess
import json
import os
import time

parser = argparse.ArgumentParser()

parser.add_argument('-s', '--status',
                    type=int,
                    default=60,
                    help='Warning time limit (seconds) for last status-update')

parser.add_argument('-f', '--failover',
                    type=int,
                    default=3600,
                    help='Warning time limit (seconds) for last failover')

parser.add_argument('-w', '--warn',
                    type=int,
                    default=7200,
                    help='Warning time limit (seconds) for maintenance duration')

parser.add_argument('-c', '--crit',
                    type=int,
                    default=21600,
                    help='Critical time limit (seconds) for maintenance duration')

parser.add_argument('-d', '--downtime',
                    type=int,
                    default=24*60*60,
                    help='Critical time limit (seconds) for service downtime')

args = parser.parse_args()

def die(code=0, comment="OK"):
    if code == 0:
        print('0;OK')
    else:
        print('%d;%s' % (code, comment))
    sys.exit(0)

try:
    devnull = open('/dev/null', 'w')

    res = subprocess.call('sudo service pgsync status', shell=True, \
                          stdout=devnull, stderr=devnull)
    service_down = res != 0

    status_filename = '/tmp/pgsync.status'
    status_file = open(status_filename, 'r')
    status = json.load(status_file)

    if service_down:
        last_modified_ts = os.stat(status_filename).st_mtime
        downtime_duration = int(time.time() - last_modified_ts)
        if downtime_duration > args.downtime:
            die(2, 'Pgsync is not running for {} seconds (threshold {}).'.format(downtime_duration, args.downtime))
        else:
            die(1, 'Pgsync is not running.')

    if status['ts'] + args.status <= time.time():
        die(1, 'Last update of status file was %d seconds ago.' % \
                int(time.time() - status['ts']))

    if status['zk_state'].get('maintenance', {}).get('status') == 'enable':
        maintenance_ts = float(status['zk_state'].get('maintenance').get('ts'))
        maintenance_duration = int(time.time()-maintenance_ts)
        if maintenance_duration > args.crit:
            die(2, 'Maintenance was enabled too long, {} seconds ago.'.format(maintenance_duration))
        elif maintenance_duration > args.warn:
            die(1, 'Maintenance was enabled, {} seconds ago.'.format(maintenance_duration))
        else:
            die()

    if status['zk_state'].get('lock_holder') is None:
        die(2, 'No one holds leader lock in ZK.')

    last = status['zk_state'].get('last_failover_time')
    if last and last + args.failover >= time.time():
        die(1, 'Last failover has been done %d seconds ago.' % \
                int(time.time() - last))

    zk_tli = status['zk_state'].get('timeline')
    db_tli = status['db_state'].get('timeline')
    if db_tli and zk_tli and db_tli != zk_tli:
        die(1, 'Timeline of PostgreSQL (%d) is not equal to ' % db_tli +
               'timeline in ZK (%d).' % zk_tli)
except Exception as err:
    die(1, str(err))
finally:
    try:
        devnull.close()
    except Exception:
        pass

die()
