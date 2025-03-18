#!/skynet/python/bin/python

import os
import sys
from optparse import OptionParser

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB


def parse_cmd():
    parser = OptionParser(usage="usage: %prog [options]")

    parser.add_option("-i", "--intlookup", type="str", dest="intlookup",
                      help="Obligatory. Intlookup to process")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if not options.intlookup:
        raise Exception("Option --intlookup is obligatory")
    if len(args):
        raise Exception("Unparsed args: %s" % args)

    options.intlookup = CURDB.intlookups.get_intlookup(os.path.basename(options.intlookup))

    return options


def adjust_instances(platinum_instances, common_instances):
    platinum_extra_instances = []
    platinum_hosts = set()
    for instance in platinum_instances:
        if instance.host in platinum_hosts:
            platinum_extra_instances.append(instance)
        else:
            platinum_hosts.add(instance.host)

    need_platinum_on_hosts = set(map(lambda x: x.host, common_instances)) - platinum_hosts

    #    print "Need ", len(need_platinum_on_hosts)

    for instance in common_instances:
        if instance.host in need_platinum_on_hosts:
            need_platinum_on_hosts.remove(instance.host)
            instance.swap_data(platinum_extra_instances.pop())


def adjust_snippets_intlookup(options):
    for switch_type in [0, 1]:
        platinum_instances = []
        common_instances = []
        for shard_id in range(options.intlookup.brigade_groups_count * options.intlookup.hosts_per_group):
            if options.intlookup.get_tier_for_shard(shard_id) == 'PlatinumTier0':
                platinum_instances.extend(options.intlookup.get_base_instances_for_shard(shard_id))
            else:
                common_instances.extend(options.intlookup.get_base_instances_for_shard(shard_id))
        platinum_instances = filter(lambda x: x.host.switch_type == switch_type, platinum_instances)
        common_instances = filter(lambda x: x.host.switch_type == switch_type, common_instances)

        #        print len(platinum_instances), len(common_instances)

        adjust_instances(platinum_instances, common_instances)


if __name__ == '__main__':
    options = parse_cmd()
    adjust_snippets_intlookup(options)
    CURDB.update()
