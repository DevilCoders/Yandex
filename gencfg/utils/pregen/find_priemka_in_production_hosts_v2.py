#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import string
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from gaux.aux_utils import get_worst_tier_by_size
from gaux.aux_shared import calc_source_hosts
from check import check_disk_size
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser("Find machines for priemka/priemka backup")
    parser.add_argument("-r", "--replicas", type=float, dest="replicas", required=True,
                        help="obligatory option. Total number of replicas")
    parser.add_argument("-t", "--tiers", type=argparse_types.tiers, dest="tiers", required=True,
                        help="obligatory option. Number of shards (instances)")
    parser.add_argument("-a", "--all-tiers-sizes", type=str, dest="all_tiers_sizes", required=True,
                        help="Obligatory option. All tiers sizes")
    parser.add_argument("-g", "--group", type=argparse_types.group, dest="group", required=True,
                        help="obligatory option. Group to get hosts from")
    parser.add_argument("-s", "--slave-group", type=argparse_types.group, dest="slave_group", required=True,
                        help="obligatory option. Slave group to add hosts to")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda, dest="flt",
                        help="Optional. Host filter")
    parser.add_argument("--slot-size", type=int, dest="slot_size", required=True,
                        help="Obligatory. Slot size (in gigabytes)")
    parser.add_argument("--master-slot-size", type=int, dest="master_slot_size", required=True,
                        help="Obligatory. Master slot size")
    parser.add_argument("--slots-per-host", type=int, dest="slots_per_host", required=True,
                        help="Obligatory. Disk slots per host")
    parser.add_argument("--excluded-groups", type=argparse_types.groups, dest="excluded_groups", default=[],
                        help="Optional. Exclude specified groups")
    parser.add_argument("--clear-current-hosts", action="store_true", dest="clear_current_hosts", default=False,
                        help="Optional. Clear current group")
    parser.add_argument("-v", "--verbose", action="store_true", default=False,
                        help="Optional. Verbose mode (show some statiscics)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if CURDB.groups.has_group(string.replace(options.slave_group.name, '_BASE', '_INT')):
        options.slave_group_int = CURDB.groups.get_group(string.replace(options.slave_group.name, '_BASE', '_INT'))
    else:
        options.slave_group_int = None

    if options.slave_group.master != options.group:
        raise Exception("Group %s is not master for group %s" % (options.group, options.slave_group))
    if options.slot_size % options.master_slot_size != 0:
        raise Exception("Slot size should be divided by master slot size")

    return options


def add_slave_hosts(hosts, total_instances, group):
    for host in hosts[:total_instances]:
        CURDB.groups.add_slave_host(host, group)


if __name__ == '__main__':
    options = parse_cmd()

    if options.clear_current_hosts:
        options.slave_group.clearHosts()

    # prepare src hosts
    hosts = calc_source_hosts([options.group], remove_slaves=False)
    if options.flt:
        hosts = filter(lambda x: options.flt(x), hosts)

    if options.verbose:
        print "At start: %s left" % len(hosts)

    hosts = set(hosts)
    for group in options.excluded_groups:
        excluded_group_hosts = group.getHosts()
        excluded_group_hosts = set(excluded_group_hosts)
        hosts -= excluded_group_hosts
    hosts = list(hosts)

    if options.verbose:
        print "After excluding from 'exclude' groups: %s left" % len(hosts)

    hosts = filter(lambda x: x.memory >= options.master_slot_size + options.slot_size, hosts)

    if options.verbose:
        print "After excluing hosts with not enough momory: %s left" % len(hosts)

    # find current used disk size
    suboptions = {
        'hosts': list(set(map(lambda x: x.host, options.group.get_instances()))),
        'verbose': False,
        'show_top': False,
        'check_ssd_size': True,
    }
    host_used_disk = check_disk_size.main(type("CheckDiskSizeOptions", (object,), suboptions)(), from_cmd=False)

    # filter by disk size
    worst_tier_by_size = get_worst_tier_by_size(options.all_tiers_sizes, CURDB)


    def get_needed_disk_size(host):
        sz = options.group.card.properties.extra_disk_size + options.slave_group.properties.extra_disk_size
        sz += options.slots_per_host * (
        worst_tier_by_size * 2 + options.slave_group.properties.extra_disk_size_per_instance) * options.slot_size / options.master_slot_size
        # FIXME
        # sz += ((host.memory - options.slot_size) / options.master_slot_size) * (worst_tier_by_size * 2 + options.group.card.properties.extra_disk_size_per_instance)
        sz += min(7, (host.memory - options.slot_size) / options.master_slot_size) * (
        worst_tier_by_size * 2 + options.group.card.properties.extra_disk_size_per_instance)

        sz += host_used_disk.get(host, 0)
        return sz


    hosts = filter(lambda x: x.disk >= get_needed_disk_size(x), hosts)
    if options.verbose:
        print "After filtering by disk: %s left" % len(hosts)

    # filter by available memory
    hosts = filter(lambda x: len(options.group.get_host_instances(x)) > options.slot_size / options.master_slot_size,
                   hosts)
    hosts.sort(cmp=lambda x, y: cmp(y.memory, x.memory))
    if options.verbose:
        print "After filtering by memory: %s left" % len(hosts)

    # check if enough hosts
    needed_instances = int(sum(map(lambda x: x.get_shards_count(), options.tiers)) * options.replicas)
    if needed_instances % options.slots_per_host == 0:
        needed_instances = needed_instances / options.slots_per_host
    else:
        needed_instances = needed_instances / options.slots_per_host + 1
    print type(needed_instances), type(options.replicas), type(options.slots_per_host), needed_instances, len(hosts)
    print "Have %d potential hosts, needed %d hosts" % (len(hosts), needed_instances)
    if needed_instances > len(hosts):
        raise Exception("Not enough hosts")

    CURDB.groups.add_slave_hosts(hosts[:needed_instances], options.slave_group)
    CURDB.groups.update()
