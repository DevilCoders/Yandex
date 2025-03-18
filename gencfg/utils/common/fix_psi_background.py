#!/skynet/python/bin/python
"""Script to remove psi from background (GENCFG-2364)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt


class EActions(object):
    FIX = 'fix'  # fix psi background groups
    ALL = [FIX]

def get_parser():
    parser = ArgumentParserExt(description='Manipulate on ipv4tunnels of on ipv6-only machines')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')

    return parser


def main_fix(options):
    # calculate groups to process
    psi_groups = [x for x in CURDB.groups.get_groups() if x.card.tags.itype == 'psi' and x.card.properties.nonsearch == False]
    no_donor_groups = [x for x in psi_groups if x.card.master is not None and x.card.host_donor is None]
    has_donor_groups = [x for x in psi_groups if x.card.host_donor is not None]
    master_groups = [x for x in psi_groups if x.card.master is None]
    master_groups = [x for x in master_groups if x.card.name not in [y.card.host_donor for y in has_donor_groups]]

    # calculate hosts to remove
    bad_groups = [x for x in CURDB.groups.get_groups() if x.card.properties.allow_background_groups == False or x.card.properties.nonsearch == True]
    bad_hosts = [x.getHosts() for x in bad_groups]
    bad_hosts = set(sum(bad_hosts, []))

    # remove from normasl slave groups
    for group in no_donor_groups:
        remove_hosts = set(group.getHosts()) & bad_hosts
        remove_hosts = sorted(remove_hosts, key=lambda x: x.name)
        if remove_hosts:
            for host in remove_hosts:
                group.removeHost(host)
            print 'Removed hosts from slave group {}: {}'.format(group.card.name, ' '.join(x.name for x in remove_hosts))

    # remove from groups with donor
    for group in has_donor_groups:
        donor_group = CURDB.groups.get_group(group.card.host_donor)
        remove_hosts = set(donor_group.getHosts()) & bad_hosts
        remove_hosts = sorted(remove_hosts, key=lambda x: x.name)
        if remove_hosts:
            for host in remove_hosts:
                donor_group.removeHost(host)
            print 'Removed hosts from donor group {}: {}'.format(donor_group.card.name, ' '.join(x.name for x in remove_hosts))

    # remove from master group
    for group in master_groups:
        remove_hosts = set(group.getHosts()) & bad_hosts
        remove_hosts = sorted(remove_hosts, key=lambda x: x.name)
        if remove_hosts:
            for host in remove_hosts:
                donor_group.removeHost(host)
            print 'Removed hosts from master group: {}'.format(group.card.name, ' '.join(x.name for x in remove_hosts))

    CURDB.update(smart=True)

def main(options):
    if options.action == EActions.FIX:
        main_fix(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
