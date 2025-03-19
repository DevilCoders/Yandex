#!/usr/bin/python

from contextlib import closing
from subprocess import check_output, check_call
from yaml import load
import argparse
import os
from sys import exit

import MySQLdb

LETTER_TO_DC = {'h': 'sas',
                'e': 'iva',
                'f': 'myt',
                'k': 'vla',
                'i': 'man'}


def get_dc_by_fqdn(fqdn):
    if fqdn[:3] in LETTER_TO_DC.values():
        return fqdn[:3]
    return LETTER_TO_DC[fqdn[fqdn.find(".db") - 1]]


def get_conn():
    return MySQLdb.connect(read_default_file=os.path.expanduser("~mysql/.my.cnf"), db='mysql')


def is_master(cur):
    cur.execute("SHOW SLAVE STATUS")
    if len(cur.fetchall()) > 0:
        return False
    return True


def get_info(from_hosts, from_dc):
    cmd = 'mysync info'
    info = load(check_output(cmd.split()))
    active_nodes = info['active_nodes']
    master = info.get('master')
    candidates = []
    for active_node in active_nodes:
        bad_node = False
        for prefix in from_hosts:
            if active_node.startswith(prefix):
                bad_node = True
        for dc in from_dc:
            if get_dc_by_fqdn(active_node) == dc:
                bad_node = True
        if not bad_node:
            candidates.append(active_node)

    candidates_lag = {x: info['health'][x]['slave_state']['replication_lag'] for x in candidates if x != master}
    if master in candidates:
        candidates_lag[master] = -1

    return candidates_lag, info['ha_nodes']


def get_args():
    parser = argparse.ArgumentParser()

    # can't use empty  string as default, because it's prefix of every string
    # no one fqdn starts with ' '
    parser.add_argument('--from-hosts',
                        type=str,
                        default=' ',
                        help="List of prefixes of affected hosts")

    parser.add_argument('--max-lag',
                        type=int,
                        default=240,
                        help="Max replication lag of a candidate for role master")

    parser.add_argument('--wait-duration',
                        type=int,
                        default=300,
                        help="How long wait for switchover to complete, 0s to return immediately (default 5m0s)")

    parser.add_argument('--dry-run',
                        help='Shows if the current host is a master.'
                             ' If it is shows fqdn of a candidate for role master',
                        action='store_true')

    parser.add_argument('--from-dc',
                        type=str,
                        default=' ',
                        help="List of affected DC's")
    parser.add_argument('--run-from-replica',
                        help="Start switchover even if script executed on replica",
                        action="store_true")

    return parser.parse_args()


if __name__ == '__main__':
    args = get_args()
    with closing(get_conn()) as conn:
        cur = conn.cursor()
        if not is_master(cur) and not args.run_from_replica:
            print('NO')
            exit(0)

    candidates_lag, ha_nodes = get_info(args.from_hosts.split(','), args.from_dc.split(','))

    if len(candidates_lag) == 0:
        message = 'There is no candidates for role master'
        if args.dry_run:
            print('YES {0}'.format(message))
            exit(0)
        else:
            print(message)
            exit(len(ha_nodes) > 1)

    candidate = min(candidates_lag, key=lambda x: candidates_lag[x])
    lag = candidates_lag[candidate]
    if lag > args.max_lag:
        message = 'Candidate for role master ({0}) has too much lag: {1}'.format(candidate, lag)
        if args.dry_run:
            print('YES {0}'.format(message))
            exit(0)
        else:
            print(message)
            exit(1)

    if lag == -1:
        message = 'No need to switch'
        if args.dry_run:
            print('YES {0}'.format(message))
        else:
            print(message)
        exit(0)

    message = 'new master: {0}'.format(candidate)
    if args.dry_run:
        print('YES {0}'.format(message))
    else:
        print(message)
        cmd = 'mysync switch --to {0} -w {1}s'.format(candidate, args.wait_duration)
        check_call(cmd.split())
