#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from optparse import OptionParser

import gencfg
from core.db import CURDB


def check_constraint(i1, i2, data):
    if i1.host.name not in data[i2.host.name] and i2.host.name != i1.host.name:  # and i2.host.switch != i1.host.switch:
        return True
    return False


def parse_cmd():
    parser = OptionParser(usage="usage: %prog [options]")

    parser.add_option("-i", "--intlookup", type="str", dest="intlookup",
                      help="first replica intlookup to perform changes")
    parser.add_option("-b", "--backup-intlookup", type="str", dest="backup_intlookup",
                      help="second replica intlookup to perform changes")
    parser.add_option("-a", "--add-ints", action="store_true", default=False, dest="add_ints",
                      help="Add ints to intlookups")
    parser.add_option("-r", "--random", action="store_true", default=False, dest="random", help="Add random factor")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if not options.intlookup:
        raise Exception("Unspecified intlookup (-i option)")
    if not options.backup_intlookup:
        raise Exception("Unspecified backup intlookup (-b option)")
    if len(args):
        raise Exception("Unknown options: %s" % (' '.join(args)))

    options.intlookup = CURDB.intlookups.get_intlookup(os.path.basename(options.intlookup))
    options.backup_intlookup = CURDB.intlookups.get_intlookup(os.path.basename(options.backup_intlookup))

    return options


if __name__ == '__main__':
    options = parse_cmd()

    # remove 2nd replica in all intlookups
    if options.random:
        import random
        import time

        random.seed(time.time())

    main_multishards = options.intlookup.get_multishards()
    backup_multishards = options.backup_intlookup.get_multishards()

    for brigade_group in main_multishards + backup_multishards:
        if not options.random:
            brigade_group.brigades = brigade_group.brigades[:1]
        else:
            max_x = len(brigade_group.brigades)
            x = random.randrange(0, max_x)
            brigade_group.brigades = [brigade_group.brigades[x]]

    # create map: instance -> tier name
    instances_tiers = {}
    for ilist, tier in options.intlookup.get_used_instances_with_tier() + options.backup_intlookup.get_used_instances_with_tier():
        for instance in ilist:
            instances_tiers[instance] = tier

    # distribute instances in backup
    data = defaultdict(list)
    backup_instances = options.backup_intlookup.get_used_base_instances()
    for i in range(options.intlookup.hosts_per_group * options.intlookup.brigade_groups_count):
        instance = options.intlookup.get_base_instances_for_shard(i)[0]

        found = False
        for j in range(len(backup_instances)):
            if instances_tiers[instance] != instances_tiers[backup_instances[j]]:
                continue
            if check_constraint(backup_instances[j], instance, data):
                data[instance.host.name].append(backup_instances[j].host.name)
                backup_multishards[i / options.intlookup.hosts_per_group].brigades[0].basesearchers[
                    i % options.intlookup.hosts_per_group] = [backup_instances.pop(j)]
                found = True
                break
        if not found:
            # some weird stuff
            backup_multishards[i / options.intlookup.hosts_per_group].brigades[0].basesearchers[
                i % options.intlookup.hosts_per_group] = [backup_instances.pop(0)]

            for j in range(i):
                backup_j = options.backup_intlookup.get_base_instances_for_shard(j)[0]
                backup_i = options.backup_intlookup.get_base_instances_for_shard(i)[0]
                if check_constraint(backup_j, instance, data) and check_constraint(
                        options.intlookup.get_base_instances_for_shard(j)[0], backup_i, data):
                    backup_j.swap_data(backup_i)
                    data[instance.host.name].append(backup_i.host.name)
                    data[options.intlookup.get_base_instances_for_shard(j)[0].host.name].append(backup_j.host.name)
                    found = True
                    break

            if not found:
                raise Exception("OOPS")

    for brigade_group in backup_multishards:
        brigade_group.brigades = brigade_group.brigades[:1]

    # if len(backup_instances) != 0:
    #    print len(backup_instances)
    #    raise Exception("OOPS")

    CURDB.update(smart=True)
