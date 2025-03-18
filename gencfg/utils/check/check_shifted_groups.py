#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import re

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types


def fix_host(host):
    """
        This function fix host for virtual machines to get template host, e. g. sas1-1382-24418.vm.search.yandex.net -> sas1-1382.search.yandex.net
    """

    m = re.match("^(.*)-\d+\.vm\.(.*)$", host.name)
    if m:
        return CURDB.hosts.get_host_by_name("%s.%s" % (m.group(1), m.group(2)))
    else:
        return host


def host_to_shards_mapping_for_intlookup(intlookup):
    """Find shards on all hosts and create mapping: <host name> -> <list of shards>"""
    mapping = defaultdict(set)

    for shard_id in xrange(intlookup.get_shards_count()):
        shard_with_tier = intlookup.get_tier_and_shard_id_for_shard(shard_id)
        for instance in intlookup.get_base_instances_for_shard(shard_id):
            mapping[fix_host(instance.host)].add(shard_with_tier)

    return mapping


def host_to_shards_mapping_for_group(group):
    """Find shards on all hosts and create mapping: <host name> -> <list of shards>"""

    assert (len(group.card.intlookups) >= 1), "Group <%s> shoud have at least one intlookup" % group.card.name

    mapping = defaultdict(set)
    for intlookup in map(lambda x: CURDB.intlookups.get_intlookup(x), group.card.intlookups):
        for k, v in host_to_shards_mapping_for_intlookup(intlookup).iteritems():
            mapping[k] |= v

    return mapping


def get_parser():
    parser = ArgumentParserExt(description='Check if shifted intlookups on same hosts as original ones')
    parser.add_argument('-g', '--groups', type=argparse_types.groups, required=True,
                        help='Obligatory. List of groups to check')
    parser.add_argument('-i', '--intlookup-pairs', type=str, default=None,
                        help='Optional. Comma-separated list of pairs of intlookups to be checked (e.g. MSK_WEB_BASE_HAMSTER.WebTier0:MSK_WEB_BASE.WebTier0,MSK_WEB_BASE_HAMSTER.PlatinumTier0:MSK_WEB_BASE.PlatinumTiero')

    return parser

def main(options):
    # process groups
    affected_groups = filter(lambda x: x.card.properties.nidx_for_group is not None, options.groups)
    affected_groups = [x for x in affected_groups if not (len(x.card.intlookups) == 0 and len(CURDB.groups.get_group(x.card.properties.nidx_for_group).card.intlookups) == 0)]
    failed_affected_groups = []

    for affected_group in affected_groups:
        template_group = CURDB.groups.get_group(affected_group.card.properties.nidx_for_group)

        affected_group_mapping = host_to_shards_mapping_for_group(affected_group)
        template_group_mapping = host_to_shards_mapping_for_group(template_group)

        for host in affected_group_mapping:
            if len(affected_group_mapping[host] - template_group_mapping[host]) > 0:
                failed_affected_groups.append(affected_group)
                break

    print "Total %d groups, failed %d groups:" % (len(affected_groups), len(failed_affected_groups))
    for group in failed_affected_groups:
        print "    Group <%s> is not shifted from <%s>" % (group.card.name, group.card.properties.nidx_for_group)

    # process intlookups
    if options.intlookup_pairs is not None:
        affected_intlookups = [(x.partition(':')[0], x.partition(':')[2]) for x in options.intlookup_pairs.split(',')]
    else:
        affected_intlookups = []
    affected_intlookups = [(CURDB.intlookups.get_intlookup(x), CURDB.intlookups.get_intlookup(y)) for (x, y) in affected_intlookups]
    failed_affected_intlookups = []

    for dst_intlookup, src_intlookup in affected_intlookups:
        dst_mapping = host_to_shards_mapping_for_intlookup(dst_intlookup)
        src_mapping = host_to_shards_mapping_for_intlookup(src_intlookup)

        for host in dst_mapping:
            if len(dst_mapping[host] - src_mapping[host]) > 0:
                failed_affected_intlookups.append((dst_intlookup, src_intlookup))
                break

    print 'Total {} intlookup, failed {} intlookups:'.format(len(affected_intlookups), len(failed_affected_intlookups))
    for dst_intlookup, src_intlookup in failed_affected_intlookups:
        print '    Intlookup <{}> is not shifted from <{}>'.format(dst_intlookup.file_name, src_intlookup.file_name)

    return (len(failed_affected_groups) > 0) or (len(failed_affected_intlookups) > 0)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    status = main(options)

    sys.exit(status)
