#!/skynet/python/bin/python
"""Replace hosts in group by hosts of different configuration (when we have no hosts with same configuration)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types


class THostFreeResources(object):
    """Class, calculating free resources of host"""

    def __init__(self, host):
        self.host = host
        self.instances = []
        self.memory = self.host.get_avail_memory()
        self.power = self.host.power

    def can_add_instance(self, instance):
        if instance.power > self.power:
            return False
        if CURDB.groups.get_group(instance.type).card.reqs.instances.memory_guarantee.value > self.memory:
            return False
        return True

    def add_instance(self, instance):
        self.instances.append(instance)
        self.power -= instance.power
        self.memory -= CURDB.groups.get_group(instance.type).card.reqs.instances.memory_guarantee.value


def get_parser():
    parser = ArgumentParserExt('Replace unworking hosts with hosts of different configuration')
    parser.add_argument('-g', '--group', type=argparse_types.group, required=True,
            help='Obligatory. Group to remove hosts from')
    parser.add_argument("-s", "--hosts", type=argparse_types.hosts, required=True,
                        help="Obligatory. List of hosts to replace")
    parser.add_argument("-r", "--src-groups", type=argparse_types.groups, default=[],
                        help="Optional. List of reserved groups (*_RESERVED by default)")
    parser.add_argument("-d", "--dest-group", type=argparse_types.group, default="ALL_UNWORKING_BUSY",
                        help="Optional. Destination group for replaced machines")
    parser.add_argument("-x", "--skip-groups", type=argparse_types.groups, default=[],
                        help="Optional. List of groups to skip")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 2.")

    return parser


def normalize(options):
    if len(options.src_groups) == 0:
        options.src_groups = [CURDB.groups.get_group(options.group.get_default_reserved_group())]


def main(options):
    avail_hosts = sum((x.getHosts() for x in options.src_groups), [])
    avail_hosts.sort(key=lambda x: x.memory)

    replace_hosts = []
    for host in options.hosts:
        if options.verbose > 0:
            print 'Replacing host {}'.format(host.name)

        host_instances = []
        for instance in CURDB.groups.get_host_instances(host):
            instance_group = CURDB.groups.get_group(instance.type)

            if instance_group in options.skip_groups:
                continue

            if (instance_group == options.group) or (instance_group.card.master == options.group):
                if instance in instance_group.get_kinda_busy_instances():
                    host_instances.append(instance)

        if options.verbose > 1:
            print '    Need to replace: {}'.format(' '.join(x.full_name() for x in host_instances))

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
