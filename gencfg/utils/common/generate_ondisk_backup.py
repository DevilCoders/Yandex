#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from utils.check import check_disk_size
from core.instances import TIntGroup, TIntl2Group, TMultishardGroup


def parse_cmd():
    parser = ArgumentParser(usage="Add intlookup for disk backup (account only disk size)")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Slave group name")
    parser.add_argument("-r", "--replicas", type=int, required=True,
                        help="Obligatory. Number of replicas")
    parser.add_argument("-m", "--max-per-host", type=int, required=True,
                        help="Obligatory. Maximum number of instances per host")
    parser.add_argument("-i", "--intlookup-name", type=str, required=True,
                        help="Obligatory. Intlookup name")
    parser.add_argument("-o", "--other-intlookups", type=argparse_types.intlookups, default=[],
                        help="Optional. Intlookups which should not 'interset' with result intlookup")
    parser.add_argument("-t", "--tiers", type=argparse_types.tiers, required=True,
                        help="Obligatory. List of tiers")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.group.card.master is None:
        raise Exception("Group %s does not have master" % options.group.card.name)

    return options


def main(options):
    # clear old stuff
    if CURDB.intlookups.has_intlookup(options.intlookup_name):
        CURDB.intlookups.remove_intlookup(options.intlookup_name)
    options.group.clearHosts()

    # fill shard hosts
    othershardhosts = defaultdict(set)
    for intlookup in options.other_intlookups:
        for i in range(intlookup.hosts_per_group * intlookup.brigade_groups_count):
            primus = intlookup.get_primus_for_shard(i)
            othershardhosts[primus] |= set(map(lambda x: x.host, intlookup.get_base_instances_for_shard(i)))

    allhosts = options.group.card.master.getHosts()
    suboptions = {
        'hosts': allhosts,
        'verbose': False,
        'show_top': False,
        'check_ssd_size': False,
    }

    disk_usage = check_disk_size.main(type("CheckDiskSizeOptions", (object,), suboptions)(), from_cmd=False)
    disk_usage = map(lambda x: (x, x.disk - disk_usage[x], 0), disk_usage.keys())
    disk_usage.sort(cmp=lambda (name1, sz1, m1), (name2, sz2, m2): cmp(sz1, sz2))

    intlookup = CURDB.intlookups.create_empty_intlookup(options.intlookup_name)
    intlookup.base_type = options.group.card.name
    intlookup.brigade_groups_count = sum(map(lambda x: x.get_shards_count(), options.tiers))
    intlookup.tiers = map(lambda x: x.name, options.tiers)
    intlookup.hosts_per_group = 1
    intlookup.intl2_groups.append(TIntl2Group())
    for shard_id in range(intlookup.brigade_groups_count):
        intlookup.intl2_groups[0].multishards.append(TMultishardGroup())

    disk_usage_index = 0
    for i in range(options.replicas):
        for j in range(intlookup.brigade_groups_count):
            primus = intlookup.get_primus_for_shard(j)
            tiername = intlookup.get_tier_for_shard(j)
            extrasz = CURDB.tiers.tiers[tiername].disk_size * options.group.card.properties.extra_disk_shards

            found = False
            for index in range(len(disk_usage)):
                realindex = (index + disk_usage_index) % len(disk_usage)

                if disk_usage[realindex][1] < extrasz or disk_usage[realindex][2] == options.max_per_host:
                    continue
                if disk_usage[realindex][0] in othershardhosts[primus]:
                    continue

                if not options.group.hasHost(disk_usage[realindex][0]):
                    options.group.addHost(disk_usage[realindex][0])

                instance = CURDB.groups.get_instance_by_N(disk_usage[realindex][0].name, options.group.card.name,
                                                          disk_usage[realindex][2])
                brigade = TIntGroup([[instance]], [])
                intlookup.intl2_groups[0].multishards[j].brigades.append(brigade)

                disk_usage[realindex] = (
                disk_usage[realindex][0], disk_usage[realindex][1] - extrasz, disk_usage[realindex][2] + 1)

                found = True
                disk_usage_index = (realindex + 1) % len(disk_usage)

                break

            if not found:
                raise Exception("Not enough instances: %d need, %d found" % (options.replicas * intlookup.brigade_groups_count, i * options.replicas + j))

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
