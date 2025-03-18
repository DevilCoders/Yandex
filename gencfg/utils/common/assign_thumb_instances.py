#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import gaux.aux_utils
import core.argparse.types as argparse_types
from core.instances import TIntGroup


def different_hosts(instances):
    if len(instances) == len(set(map(lambda x: x.host, instances))):
        return True
    return False


def different_switches(instances):
    if len(instances) == len(set(map(lambda x: x.host.switch, instances))):
        return True
    return False


def different_dcs(instances):
    if len(instances) == len(set(map(lambda x: x.host.dc, instances))):
        return True
    return False


CHECKERS = {
    'difhosts': different_hosts,
    'difswitches': different_switches,
    'difdcs': different_dcs,
}


def parse_cmd():
    parser = ArgumentParser(description="Recluster thumbs inside web")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group name")
    parser.add_argument("-r", "--replicas", type=int, required=True,
                        help="Obligatory. Number of replicas required")
    parser.add_argument("-t", "--tier", type=str, required=True,
                        help="Obligatory. Tier")
    parser.add_argument("-c", "--checkers", type=argparse_types.comma_list, default=[],
                        help="Optional. Additional checkers/constraints on replicas: one or more from %s" % ",".join(
                            CHECKERS.keys()))

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    for checker in options.checkers:
        if checker not in CHECKERS:
            raise Exception("Unknown checker %s" % checker)

    if len(options.group.card.intlookups) > 1:
        raise Exception("More than 1 intlookup in group %s" % options.group.card.name)

    return options


def add_replica_to_shard(intlookup, shard_id, avail_instances, mygroup_instances_with_shards, options):
    current_replicas = intlookup.get_base_instances_for_shard(shard_id)

    remove_from_avail = []

    found = False
    for instance in avail_instances:
        # check if we have enough disk
        host = instance.host
        host_instances = mygroup_instances_with_shards[host]
        needed_disk = gaux.aux_utils.get_used_disk(host_instances + [(options.tier, shard_id, options.group.card.name)],
                                                  CURDB)

        if needed_disk > host.disk:
            remove_from_avail.append(instance)
            continue

        # check for checkers
        failed = False
        for checker in options.checkers:
            if not CHECKERS[checker](current_replicas + [instance]):
                failed = True
                break
        if failed:
            continue

        host_instances.append((options.tier, shard_id, options.group.card.name))
        avail_instances.remove(instance)
        intlookup.brigade_groups[shard_id].brigades.append(TIntGroup([[instance]], []))
        found = True
        break

    if len(remove_from_avail) > 0:
        print "Unavail %s" % ",".join(map(lambda x: x.name(), remove_from_avail))

    avail_instances -= set(remove_from_avail)

    return found


def main(options):
    intlookup = CURDB.intlookups.get_intlookup(options.group.card.intlookups[0])

    # find current mapping: host -> [(tier, shard_id, group_name), ...]
    mygroup_instances_with_shards = {}
    for host in options.group.getHosts():
        mygroup_instances_with_shards[host] = []

    instance_shards = CURDB.intlookups.build_shard_mapping()
    for instance in instance_shards:
        if instance.host in mygroup_instances_with_shards:
            tier, shard_id = instance_shards[instance]
            mygroup_instances_with_shards[instance.host].append((tier, shard_id, instance.type))

    avail_instances = set(options.group.get_instances()) - set(options.group.get_busy_instances())

    # iterate all shards and fill absent replicas
    for shard_id in range(intlookup.brigade_groups_count):
        while len(intlookup.get_base_instances_for_shard(shard_id)) < options.replicas:
            status = add_replica_to_shard(intlookup, shard_id, avail_instances, mygroup_instances_with_shards, options)
            if not status:
                raise Exception("Failed to add replica %s to shard %s" % (len(intlookup.get_base_instances_for_shard(shard_id)), shard_id))

    CURDB.update()


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
