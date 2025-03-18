#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
import copy

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.instances import TIntGroup, TMultishardGroup, TIntl2Group


def parse_cmd():
    parser = ArgumentParser(description="Distribute hosts to replicas for YASM_LINES (example in GENCFG-267)")
    parser.add_argument("-f", "--first-replica", type=argparse_types.comma_list, required=True,
                        help="Obligatory. File with hosts for first replica")
    parser.add_argument("-s", "--second-replica", type=argparse_types.comma_list, required=True,
                        help="Obligatory. File with hosts for second replica")
    parser.add_argument("-g", "--group", type=argparse_types.group, default=None,
                        help="Optional. Group to process (if not specified, will be detected automatically based on hosts lists)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def generate_intlookup(group, neighbours):
    assert len(group.card.intlookups) <= 1
    assert len(group.get_instances()) % 2 == 0
    assert len(group.get_instances()) == len(group.getHosts())

    filtered_neighbours = filter(lambda x: group.hasHost(x[0]) and group.hasHost(x[1]), neighbours)

    assert len(filtered_neighbours) * 2 == len(group.getHosts())

    if len(group.card.intlookups) == 1:
        intlookup = CURDB.intlookups.get_intlookup(group.card.intlookups[0])
        for brigade_group in intlookup.brigade_groups:
            brigade_group.brigades = []
    else:
        fname = group.card.name
        intlookup = CURDB.intlookups.create_empty_intlookup(fname)
        intlookup.brigade_groups_count = len(group.get_instances()) / 2
        intlookup.tiers = None
        intlookup.hosts_per_group = 1
        intlookup.base_type = group.card.name

        intl2_group = TIntl2Group()
        for i in range(intlookup.brigade_groups_count):
            intl2_group.multishards.append(TMultishardGroup())
        intlookup.intl2_groups.append(intl2_group)

    multishards = intlookup.get_multishards()
    for i, (fhost, shost) in enumerate(filtered_neighbours):
        multishards[i].brigades.append(TIntGroup([[group.get_host_instances(fhost)[0]]], []))
        multishards[i].brigades.append(TIntGroup([[group.get_host_instances(shost)[0]]], []))

    intlookup.mark_as_modified()
    group.mark_as_modified()

    print "Regenerated intlookup %s for group %s" % (intlookup.file_name, group.card.name)


def normalize(options):
    if len(options.first_replica) != len(options.second_replica):
        raise Exception("Non-equal number of hosts in first replica and second replica")

    if options.group is None:
        groups = CURDB.groups.get_host_groups(options.first_replica[0])
        options.group = filter(lambda x: x.master is None and x.on_update_trigger is None, groups)[0]

    options.first_replica = CURDB.hosts.get_hosts_by_name(options.first_replica)
    options.second_replica = CURDB.hosts.get_hosts_by_name(options.second_replica)

    extra_in_group = set(options.group.getHosts()) - set(options.first_replica + options.second_replica)
    if len(extra_in_group) > 0:
        raise Exception("Group has hosts not mentioned in first/second replica: %s" % ",".join(map(lambda x: x.name(), extra_in_group)))
    extra_in_input = set(options.first_replica + options.second_replica) - set(options.group.getHosts())
    if len(extra_in_input) > 0:
        raise Exception("Hosts from input not found in group <%s>: %s" % (options.group.card.name, ",".join(map(lambda x: x.name(), options.first_replica + options.first_replica))))


def main(options):
    neighbours = zip(options.first_replica, options.second_replica)

    for group in [options.group] + options.group.slaves:
        generate_intlookup(group, neighbours)

    CURDB.update(smart=1)


if __name__ == '__main__':
    options = parse_cmd()

    normalize(options)

    main(options)
