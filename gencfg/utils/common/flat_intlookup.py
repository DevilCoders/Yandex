#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.instances import TMultishardGroup, TIntGroup, TIntl2Group
import core.argparse.types as argparse_types
from core.argparse.parser import ArgumentParserExt


def get_parser():
    parser = ArgumentParserExt(
        description="Flat or unflat intlookups (change brigade group size to 1 or arbitrary value)")

    parser.add_argument("-a", "--action", type=str, dest="action", default=None,
                        choices=["flat", "unflat"],
                        help="Obligatory. Action to execute: flat, unlat")
    parser.add_argument("-i", "--intlookup", type=argparse_types.intlookup, dest="intlookup", default=None,
                        help="Obligatory. Intlookup name")
    parser.add_argument("--process-intl2", action="store_true", default=False,
                        help="Optional. By default we flat/unflat intl1 groups. With this options we flat/unflat intl2 groups")
    parser.add_argument("--strict", action="store_true", default=False,
                        help="Optional. Strict generation (check some non-necessary requirements like equal number of multishards in every intl2 group")
    parser.add_argument("-o", "--hosts-per-group", type=argparse_types.primus_int_count, dest="hosts_per_group",
                        default=None,
                        help="Obligatory. Size of IntGroup (when unflatting intL1, or size of intL2 group when unflatting intL2)")
    parser.add_argument("--cut-extra", action="store_true", default=False,
                        help="Optional. Cut extra instances (if some brigade groups have more replicas than other just drop excess ones")

    return parser


def normalize(options):
    if options.action == "unflat" and options.hosts_per_group is None:
        raise Exception("You must specify --hosts-per-group with <unflat> mode")

    if options.action == "flat":
        if options.process_intl2 == True and len(options.intlookup.intl2_groups) == 1:
            raise Exception("Trying to flat inlt2 with already having only one intl2 group")
        if options.process_intl2 == False and options.intlookup.hosts_per_group == 1:
            raise Exception("Trying to flat group with already 1 shard per group")

    if options.action == "unflat":
        if options.process_intl2 == True and len(options.intlookup.intl2_groups) > 1:
            raise Exception("Trying to unflat intl2 with already having more than one intl2 group")
        if options.process_intl2 == False and options.intlookup.hosts_per_group > 1:
            raise Exception("Trying to unflat group with more than 1 shard per group")

    if options.strict:
        if options.action == "unflat" and options.process_intl2 == True:
            if len(options.intlookup.get_multishards()) % options.hosts_per_group != 0:
                raise Exception("Option --strict enabled and hosts_per_group in intl2 groups differ from each other")


def flat_multishard_group(multishard_group, hosts_per_group):
    """
        Functions convert single multishards group into <hosts_per_group> multishard groups, each consisting of single shard

        :type multishard_group: core.instances.TMultishardGroup
        :type hosts_per_group: int

        :param multishard_group: multishard group
        :param hosts_per_group: number of shards in multishard group (needed only if multishard_group does not have any intgroup and we can not calculate hosts_per_group)

        :return (list of core.instances.TMultishardGroup): list of multishard groups, each consisting of single shard
    """

    new_multishard_groups = map(lambda x: TMultishardGroup(), xrange(hosts_per_group))
    for intgroup in multishard_group.brigades:
        for i in range(options.intlookup.hosts_per_group):
            new_intgroup = TIntGroup([intgroup.basesearchers[i]], intgroup.intsearchers if i == 0 else [])
            new_multishard_groups[i].brigades.append(new_intgroup)

    return new_multishard_groups


def unflat_multishard_groups(multishard_groups, hosts_per_group):
    """
        Revert to flat_multishard_group function, but it returns several multishard groups

        :type multishard_groups: list[core.instances.TMultishardGroup]
        :type hosts_per_group: int

        :param multishard_groups: list of input multishard groups
        :param hosts_per_group: number of shards in result multishard group

        :return (list of core.instances.TMultishardGroup): list of multishard groups, each consisting of <hosts_per_group> shards
    """

    # check constraints
    for multishard_group in multishard_groups:
        if len(multishard_group.brigades) > 0 and len(multishard_group.brigades[0].basesearchers) != 1:
            raise Exception("Found <%d> shards in multishard group (should be exactly 1)" % (len(multishard_group.brigades[0])))
    if len(multishard_groups) % hosts_per_group != 0:
        raise Exception("<%d> multishard group is not divided by <%d> hosts_per_group" % (len(multishard_groups), hosts_per_group))

    new_multishard_groups = []
    for i in range(len(multishard_groups) / hosts_per_group):
        new_multishard_groups.append(TMultishardGroup())

        slice_multishard_groups = multishard_groups[(i * hosts_per_group):((i + 1) * hosts_per_group)]

        # check number of replicas for slice
        replicas = list(set(map(lambda x: len(x.brigades), slice_multishard_groups)))
        if len(replicas) > 1:
            raise Exception("Found groups with different number of replicas")

        for j in range(replicas[0]):
            old_intgroups = map(lambda x: x.brigades[j], slice_multishard_groups)
            old_basesearchers = sum(map(lambda x: x.basesearchers, old_intgroups), [])
            old_intsearchers = sum(map(lambda x: x.intsearchers, old_intgroups), [])
            new_intgroup = TIntGroup(old_basesearchers, old_intsearchers)
            new_multishard_groups[-1].brigades.append(new_intgroup)

    return new_multishard_groups


def main(options):
    if options.action == "flat":
        if options.process_intl2:  # flattint intl2 is very simple
            options.intlookup.intl2_groups[0].multishards = options.intlookup.get_multishards()
            options.intlookup.intl2_groups[0].intl2searchers = sum(
                map(lambda x: x.intl2searchers, options.intlookup.intl2_groups), [])
            options.intlookup.intl2_groups = options.intlookup.intl2_groups[:1]
        else:
            for intl2_group in options.intlookup.intl2_groups:
                intl2_group.multishards = sum(
                    map(lambda x: flat_multishard_group(x, options.intlookup.hosts_per_group), intl2_group.multishards),
                    [])

            options.intlookup.brigade_groups_count = sum(
                map(lambda x: len(x.multishards), options.intlookup.intl2_groups))
            options.intlookup.hosts_per_group = 1
    elif options.action == "unflat":
        if options.process_intl2:
            multishards = options.intlookup.get_multishards()
            N = max(len(multishards) / options.hosts_per_group, 1)  # number of new intl2 groups
            new_intl2_groups = []
            for i in range(N):
                fst = i * len(multishards) / N
                lst = (i + 1) * len(multishards) / N
                new_intl2_groups.append(TIntl2Group(multishards[fst:lst]))

            options.intlookup.intl2_groups = new_intl2_groups
        else:
            if options.cut_extra:
                multishards = options.intlookup.get_multishards()
                sz = min(map(lambda x: len(x.brigades), multishards))
                for multishard in multishards:
                    multishard.brigades = multishard.brigades[:sz]

            for intl2_group in options.intlookup.intl2_groups:
                intl2_group.multishards = unflat_multishard_groups(intl2_group.multishards, options.hosts_per_group)
            # do not know what to do with intl2searchers

            options.intlookup.brigade_groups_count /= options.hosts_per_group
            options.intlookup.hosts_per_group = options.hosts_per_group
    else:
        raise Exception("Unknown action %s" % options.action)

    options.intlookup.mark_as_modified()

    CURDB.intlookups.update(smart=True)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
