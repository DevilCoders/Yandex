#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from config import TAG_PATTERN
from core.argparse.parser import ArgumentParserExt
from core.db import DB, CURDB
import core.argparse.types as argparse_types
from gaux.aux_repo import get_last_tag
from core.instances import TMultishardGroup, TIntGroup, TIntl2Group
from utils.common import remove_group_unused_hosts


class ERestoreParts(object):
    HOSTS = "hosts"
    INTLOOKUPS = "intlookups"
    ALL = [HOSTS, INTLOOKUPS]


def get_parser():
    parser = ArgumentParserExt(description="Restore intlookup from olddb (keep instances, which exists in olddb)")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group to process")
    parser.add_argument("-r", "--restore-parts", type=argparse_types.comma_list, required=True,
                        help="Obligatory. Comma-separated list of parts to restore: one or several from %s" % ','.join(
                            ERestoreParts.ALL))
    parser.add_argument("-b", "--olddb", type=argparse_types.gencfg_prev_db,
                        default=DB("tag@%s" % get_last_tag(TAG_PATTERN, CURDB.get_repo())),
                        help="Optional. Restore from specified tag (otherwise restores from last tag)")
    parser.add_argument("-f", "--hosts-filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Custom filter on hosts which can be used in group")
    parser.add_argument("--strict", action="store_true", default=False,
                        help="Optional. More strictness: ensure weights are same")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Increase output verbosity (maximum 1)")

    return parser


def normalize(options):
    pass


def main(options):
    newgroup = options.group
    if len(newgroup.card.intlookups) > 0:
        for intlookup_name in newgroup.card.intlookups:
            CURDB.intlookups.remove_intlookup(intlookup_name)
    oldgroup = options.olddb.groups.get_group(newgroup.card.name)

    if ERestoreParts.HOSTS in options.restore_parts:
        old_host_names = map(lambda x: x.name, oldgroup.getHosts())

        # filter hosts by having in db
        candidates = CURDB.hosts.get_hosts_by_name(filter(lambda x: CURDB.hosts.has_host(x), old_host_names))

        # filter hosts by master
        if newgroup.card.master is not None:
            assert newgroup.card.host_donor is None, "Group <%s> has non-empty host donor <%s>" % (
            newgroup.card.name, newgroup.card.host_donor)
            candidates = list(set(candidates) & set(newgroup.card.master.getHosts()))

        # filter hosts by custom filter
        if options.hosts_filter is not None:
            candidates = {x for x in candidates if options.hosts_filter(x)}

        # add hosts to my group
        if newgroup.card.master is None:
            CURDB.groups.move_hosts(candidates, newgroup)
        else:
            CURDB.groups.add_slave_hosts(candidates, newgroup)

        newgroup.mark_as_modified()

        if options.verbose >= 1:
            print "Restored %d of %d hosts in group %s" % (len(candidates), len(old_host_names), newgroup.card.name)

    if ERestoreParts.INTLOOKUPS in options.restore_parts:
        newgroup_instances = dict(map(lambda x: ((x.host.name, x.port), x), newgroup.get_instances()))

        assert (len(oldgroup.card.intlookups) > 0)

        for oldintlookup in map(lambda x: options.olddb.intlookups.get_intlookup(x), oldgroup.card.intlookups):
            newintlookup = CURDB.intlookups.create_empty_intlookup(oldintlookup.file_name)
            newintlookup.brigade_groups_count = oldintlookup.brigade_groups_count
            newintlookup.tiers = oldintlookup.tiers
            newintlookup.hosts_per_group = oldintlookup.hosts_per_group
            newintlookup.base_type = bytes(oldintlookup.base_type)
            CURDB.intlookups.update_intlookup_links(newintlookup.file_name)

            assert newintlookup.hosts_per_group == 1  # FIXME: support groups with ints/multiple instances per group

            restored_N = 0
            all_N = 0
            multishards = []
            for shard_id in range(oldintlookup.get_shards_count()):
                brigade_group = TMultishardGroup()
                for instance in oldintlookup.get_base_instances_for_shard(shard_id):
                    all_N += 1

                    new_instance = newgroup_instances.get((instance.host.name, instance.port), None)

                    if new_instance is None:
                        continue
                    if options.strict and new_instance.power != instance.power:
                        continue

                    restored_N += 1

                    brigade = TIntGroup([[new_instance]], [])
                    brigade_group.brigades.append(brigade)
                multishards.append(brigade_group)
            newintlookup.intl2_groups.append(TIntl2Group(multishards=multishards))

            newintlookup.mark_as_modified()
            if options.verbose >= 1:
                print "Restored %d of %d instances in intlookup %s" % (restored_N, all_N, newintlookup.file_name)

        if ERestoreParts.HOSTS in options.restore_parts:
            # clean hosts which are not in intlookups
            remove_group_unused_hosts.jsmain({"group": options.group, "apply": False})

        newgroup.mark_as_modified()

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
