#!/skynet/python/bin/python
"""
    Script to find not unique vlan IPs.
"""
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

import json
import logging

import core.argparse.types as argparse_types

from core.db import CURDB
from core.argparse.parser import ArgumentParserExt


def get_parser():
    parser = ArgumentParserExt(description='Tool to find not unique vlan IPs.')
    parser.add_argument('--db', type=argparse_types.gencfg_db, default=CURDB,
                        help='Optional. Path to db')
    parser.add_argument('-g', '--groups', type=argparse_types.groups, default=list(),
                        help='Optional. Group to calc not unique IPs')
    parser.add_argument('-a', '--need-all-stat', action='count', default=0,
                        help='Optional. Dump all stats?')
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='Optional. Increase output verbosity (maximum 1)')
    return parser


def main(options):
    if options.verbose:
        logging.basicConfig(level=logging.INFO)
    else:
        logging.basicConfig(level=logging.ERROR)

    ip_to_host = {'vlan688': {}, 'vlan788': {}}
    for host in options.db.hosts.get_all_hosts():
        vlan688 = host.vlans.get('vlan688')
        if vlan688 is not None:
            if vlan688 not in ip_to_host['vlan688']:
                ip_to_host['vlan688'][vlan688] = []
            ip_to_host['vlan688'][vlan688].append(host.name)

        vlan788 = host.vlans.get('vlan788')
        if vlan788 is not None:
            if vlan788 not in ip_to_host['vlan788']:
                ip_to_host['vlan788'][vlan788] = []
            ip_to_host['vlan788'][vlan788].append(host.name)

    not_uniq_ip_to_host = {
        'vlan688': {k: v for k, v in ip_to_host['vlan688'].items() if len(v) > 1},
        'vlan788': {k: v for k, v in ip_to_host['vlan788'].items() if len(v) > 1},
    }
    not_uniq_ip_to_host['vlan688count'] = len(not_uniq_ip_to_host['vlan688'])
    not_uniq_ip_to_host['vlan788count'] = len(not_uniq_ip_to_host['vlan788'])

    not_uniq_groups = {'vlan688': [], 'vlan788': []}
    not_uniq_groups_hosts = {'vlan688': [], 'vlan788': []}

    for group in options.groups:
        for host in group.getHosts():
            vlan688 = host.vlans.get('vlan688')
            if vlan688 in not_uniq_ip_to_host['vlan688']:
                if host.name not in not_uniq_groups_hosts['vlan688']:
                    not_uniq_groups_hosts['vlan688'].append((host.name, vlan688))
                if group.card.name not in not_uniq_groups['vlan688']:
                    not_uniq_groups['vlan688'].append(group.card.name)

            vlan788 = host.vlans.get('vlan788')
            if vlan788 in not_uniq_ip_to_host['vlan788']:
                if host.name not in not_uniq_groups_hosts['vlan788']:
                    not_uniq_groups_hosts['vlan788'].append((host.name, vlan788))
                if group.card.name not in not_uniq_groups['vlan788']:
                    not_uniq_groups['vlan788'].append(group.card.name)

    not_uniq_groups['vlan688count'] = len(not_uniq_groups['vlan688'])
    not_uniq_groups['vlan788count'] = len(not_uniq_groups['vlan788'])
    not_uniq_groups_hosts['vlan788count'] = len(not_uniq_groups_hosts['vlan788'])
    not_uniq_groups_hosts['vlan788count'] = len(not_uniq_groups_hosts['vlan788'])

    result = {}
    if options.need_all_stat:
        result['not_uniq_ip_to_host'] = not_uniq_ip_to_host
    if not_uniq_groups_hosts:
        result['not_uniq_groups_hosts'] = not_uniq_groups_hosts
    if not_uniq_groups:
        result['not_uniq_groups'] = not_uniq_groups

    print(json.dumps(result, indent=4))


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
