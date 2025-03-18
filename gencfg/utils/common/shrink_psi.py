#!/skynet/python/bin/python
"""Shrink psi groups by removing them from bad hosts"""


import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from aux.aux_colortext import red_text


def get_parser():
    parser = ArgumentParserExt(description="Perform various actions with sandbox schedulers")
    parser.add_argument('-g', '--groups', type=core.argparse.types.groups, required=True,
            help='Obligatory. List of groups to search for psi ones (use <ALL> for all group)')
    parser.add_argument('-y', '--apply', action='store_true', default=False,
            help='Optional. Apply changes')

    return parser


def main(options):
    # filter out bad groups
    before_count = len(options.groups)
    groups = [x for x in options.groups if x.card.host_donor is None and x.card.tags.itype == 'psi' and x.card.properties.background_group is True]
    print 'Processing {} groups (filterd out {} groups): {}'.format(len(groups), before_count - len(groups), ','.join(x.card.name for x in groups))

    # calculate bad hosts
    bad_groups = [x for x in CURDB.groups.get_groups() if x.card.properties.allow_background_groups == False or x.card.properties.nonsearch == True]
    bad_hosts = [x.getHosts() for x in bad_groups]
    bad_hosts = set(sum(bad_hosts, []))

    # main cycle
    for group in groups:
        remove_hosts = sorted(set(group.getHosts()) & bad_hosts, key=lambda x: x.name)
        print 'Group {} remove {} hosts: {}'.format(group.card.name, len(remove_hosts), ','.join(x.name for x in remove_hosts))
        if len(remove_hosts) > 0:
            CURDB.groups.remove_slave_hosts(remove_hosts, group)

    if options.apply:
        CURDB.update(smart=True)
    else:
        print red_text('Not applied. Add <--apply> option to apply')


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
