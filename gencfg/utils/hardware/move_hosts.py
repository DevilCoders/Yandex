#!/skynet/python/bin/python
# coding: utf8
from __future__ import print_function

import os
import sys
import time
import logging
import functools
import collections

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import core.argparse.types as argparse_types


from argparse import ArgumentParser
from core.db import CURDB


def work_time(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        r_value = func(*args, **kwargs)
        logging.debug('WORKTIME {} {}s'.format(func.__name__, time.time() - start))
        return r_value
    return wrapper


class EAction(object):
    MOVE_HOSTS = 'move_hosts'
    MOVE_HOSTS_TO_RESERVE = 'move_hosts_to_reserve'
    ALL = [MOVE_HOSTS, MOVE_HOSTS_TO_RESERVE]


def move_hosts(hosts, group, exclude_hosts, filter_hosts, db=CURDB):
    group_hosts = group.getHosts()
    filtered_exclude_hosts = set(hosts).difference(set(exclude_hosts)).difference(group_hosts)

    for host in filtered_exclude_hosts:
        if not filter_hosts(host):
            logging.info('SKIP move {} by filter'.format(host.name))
            continue

        db.groups.move_host(host, group)
        logging.info('MOVE {} to {}'.format(host.name, group.card.name))

        for changed_group in db.groups.get_host_groups(host):
            changed_group.mark_as_modified()
        group.mark_as_modified()


def move_hosts_to_reserve(hosts, exclude_hosts, filter_hosts, db=CURDB):
    reserved = collections.defaultdict(list)
    for host in hosts:
        dc = host.dc.upper()

        reserved_group = '{}_RESERVED'.format(dc if dc not in ('IVA', 'MYT') else 'MSK')

        if host.ncpu > 32 and dc != "VLA" or host.model == "E5-2667v2" and dc not in ('IVA', 'MYT'):
            reserved_group = "RESERVE_SELECTED"

        reserved[reserved_group].append(host)

    for reserved_group, reserved_hosts in reserved.items():
        move_hosts(
            hosts=reserved_hosts,
            group=db.groups.get_group(reserved_group),
            exclude_hosts=exclude_hosts,
            filter_hosts=filter_hosts,
            db=db
        )


def parse_cmd():
    parser = ArgumentParser(description='Script to move hosts between groups by params')
    parser.add_argument('-a', '--action', type=str, choices=EAction.ALL, default=EAction.MOVE_HOSTS)
    parser.add_argument('--db', type=argparse_types.gencfg_db, default=CURDB, help='Path to GenCfg DB')

    parser.add_argument('-s', '--hosts', type=argparse_types.hosts, required=True, help='List of hosts (fqdn, invnum, groups) to move')
    parser.add_argument('-g', '--group', type=argparse_types.group_t, default=None, help='Name of destanation group')

    parser.add_argument('--exclude-hosts', type=argparse_types.hosts_t, default=set(),  help='Optional. Hosts will be excluded')
    parser.add_argument('--filter-hosts', type=argparse_types.pythonlambda, default=lambda host: True, help="Optional. Hosts filter.")

    parser.add_argument('-v', '--verbose', action='store_true', default=False, help='Optional. Show proccess info.')
    parser.add_argument('-y', '--apply', action='store_true', default=False, help='Optional. Write change to GenCfg DB.')

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.action == EAction.MOVE_HOSTS and options.group is None:
        raise ValueError('For action {} need set --group args.'.format(options.action))

    return options


def main():
    options = parse_cmd()
    if options.verbose:
        logging.basicConfig(level=logging.INFO)

    if options.action == EAction.MOVE_HOSTS:
        move_hosts(
            hosts=options.hosts,
            group=options.group,
            exclude_hosts=options.exclude_hosts,
            filter_hosts=options.filter_hosts,
            db=options.db
        )
    elif options.action == EAction.MOVE_HOSTS_TO_RESERVE:
        move_hosts_to_reserve(
            hosts=options.hosts,
            exclude_hosts=options.exclude_hosts,
            filter_hosts=options.filter_hosts,
            db=options.db
        )

    if options.apply:
        options.db.groups.update(smart=True)

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)

