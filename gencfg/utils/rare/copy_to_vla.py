#!/skynet/python/bin/python
"""Copy group from man to vla"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from core.card.node import CardNode


class EActions(object):
    SIMPLEMASTER = "simplemaster"  # move simple master group with slaves (every slave has master donor)
    DONOREDSLAVE = "donoredslave"  # move slave group with donor group already exist
    ALL = [SIMPLEMASTER, DONOREDSLAVE]

def get_parser():
    parser = ArgumentParserExt(description="Script to add hosts to Vla")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices = EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-g", "--groups", type=core.argparse.types.groups, required=True,
                        help="Obligatory. Groups to move from man to Vla")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbose level (specify multiple times to increase verbosity")

    # SIMPLEMASTER specific optons
    parser.add_argument("--min-hosts", default=1,
                        help="Optional. Minimal number of hosts in group")

    return parser


def main_donoredslave(options):
    for copy_group in options.groups:
        # check if group satisfy restrictions
        if not copy_group.card.name.startswith('MAN_'):
            raise Exception('Group {} does not start with <MAN_>'.format(copy_group.card.name))
        if copy_group.card.host_donor is None:
            raise Exception('Group {} does not have donor'.format(copy_group.card.name))
        new_donor_name = copy_group.card.host_donor.replace('MAN_', 'VLA_')
        if not CURDB.groups.has_group(new_donor_name):
            raise Exception('Donor group {} does not exists'.format(new_donor_name))

        # copy group
        new_name = copy_group.card.name.replace('MAN_', 'VLA_')
        if options.verbose > 0:
            print 'Copy group {} -> {} (new donor {})'.format(copy_group.card.name, new_name, new_donor_name)
        CURDB.groups.copy_group(copy_group.card.name, new_name, parent_name=CURDB.groups.get_group(new_donor_name).card.master.card.name, donor_name=new_donor_name)


def main_simplemaster(options):
    for copy_group in options.groups:
        # check if group satisfy restrictions
        if not copy_group.card.name.startswith('MAN_'):
            raise Exception('Group {} does not start with <MAN_>'.format(copy_group.card.name))
        for slave in copy_group.card.slaves:
            if slave.card.host_donor != copy_group.card.name:
                raise Exception('Slave group {} of {} has donor {}'.format(slave.card.name, copy_group.card.name, slave.card.host_donor))

        # create new group with all slaves
        new_name = copy_group.card.name.replace('MAN_', 'VLA_')
        if options.verbose > 0:
            print 'Copy group {} -> {}'.format(copy_group.card.name, new_name)

        if len(copy_group.card.slaves):
            slaves_mapping = ','.join(['{}:{}'.format(x.card.name, x.card.name.replace('MAN_', 'VLA_')) for x in copy_group.card.slaves])
        else:
            slaves_mapping = None
        if options.verbose > 0:
            print '    Slaves mapping: {}'.format(slaves_mapping)

        new_group = CURDB.groups.copy_group(copy_group.card.name, new_name, slaves_mapping=slaves_mapping)

        # create recluster command and add to group card
        needed_power = int(sum((x.power for x in copy_group.get_kinda_busy_instances())))
        needed_power = max(1, needed_power)
        if options.verbose > 0:
            print '   Required resources: {} power'.format(needed_power,)
        command = './utils/pregen/find_machines.py -p %s -t {{group.card.name}} -g {{reserved_group.card.name}} -S 1 --move-to-group {{group.card.name}}' % (needed_power)
        node = CardNode()
        node.id = '0'
        node.command = command
        node.prerequisites = []
        new_group.card.recluster.alloc_hosts = [node]


def main(options):
    if options.action == EActions.SIMPLEMASTER:
        main_simplemaster(options)
    elif options.action == EActions.DONOREDSLAVE:
        main_donoredslave(options)
    else:
        raise Exception('Unknown action {}'.format(options.action))

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
