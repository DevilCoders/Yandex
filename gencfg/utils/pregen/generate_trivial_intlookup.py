#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import random

import gencfg
from core.db import CURDB
from core.instances import TIntGroup, TIntl2Group, TMultishardGroup
import core.argparse.types
from core.argparse.parser import ArgumentParserExt


def get_parser():
    parser = ArgumentParserExt(description="Generate or regenerate trivial intlookup")
    parser.add_argument("-g", "--groups", type=core.argparse.types.groups, default=[],
                        help="Obligatory (without -r). Groups to get instances from")
    parser.add_argument("--db", type=core.argparse.types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-b", "--bases-per-group", type=int, default=1,
                        help="Obligatory (without -r). Hosts per group")
    parser.add_argument("-s", "--shard-count", type=str, default=None,
                        help="Obligatory (without -r). Number of shards")
    parser.add_argument("-m", "--min-replicas", type=int, default=None,
                        help="Optional. Minimal number of replicas")
    parser.add_argument("-M", "--max-replicas", type=int, default=None,
                        help="Optional. Maximal number of replicas")
    parser.add_argument("-o", "--output-file", type=str, dest="output_file", default=None,
                        help="Obligatory. Output file")
    parser.add_argument("-f", "--instance-filter", type=core.argparse.types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on instances")
    parser.add_argument("-r", "--regenerate", action="store_true", default=False,
                        help="Optional. Regenerate current intlookup")
    parser.add_argument("--shuffle", action="store_true", default=False,
                        help="Optional. Shuffle instances before generation")
    parser.add_argument("--uniform-by-host", action="store_true", default=False,
                        help="Optional. Make equal number of instances on every host (if possible)")
    parser.add_argument("-k", "--keep-existing-intlookups", action="store_true", default=False,
                        help="Optional. Keep existing intlookups (incompatable with regenerate option)")
    parser.add_argument("-e", "--create-empty-intlookup", action="store_true", default=False,
                        help="Optional. Create empty intlookup")
    parser.add_argument("--no-apply", action="store_true", default=False,
                        help="Optional. Do not apply changes")

    return parser


def normalize(options):
    if options.output_file is None and options.groups == []:
        raise Exception("You must specify --output-file or --groups")
    if options.keep_existing_intlookups and options.regenerate:
        raise Exception("Options <--keep-existing-intlookups> and <--regenerate> are incompatible")

    if options.regenerate:
        if options.bases_per_group is not None or options.shard_count is not None:
            raise Exception("Option --regenerate is incompatible with ['--bases_per_group', '--shard_count']")
        if not ((options.groups == []) ^ (options.output_file is None)):
            raise Exception("You must specify exactly one of '--groups' '--output-file options'")

        if options.groups:
            options.output_file = options.groups[0].intlookups[0]
        else:
            options.groups = [
                options.db.groups.get_group(options.db.intlookups.get_intlookup(options.output_file).base_type)]

        options.bases_per_group = options.db.intlookups.get_intlookup(options.output_file).hosts_per_group
        options.shard_count = '+'.join(options.db.intlookups.get_intlookup(options.output_file).tiers)
    else:
        if options.groups == [] or options.shard_count is None:
            raise Exception("You must specify options: --groups --shard-count")

        if options.output_file is None:
            if len(options.groups) != 1:
                raise Exception("Can not autodetect intlookup file name, because more than 1 group specified")
            options.output_file = options.groups[0].card.name

    if not options.keep_existing_intlookups:
        other_intlookup_names = sum(map(lambda x: x.card.intlookups, options.groups), [])
        other_intlookup_names = filter(lambda x: x != options.output_file, other_intlookup_names)
        for other_intlookup_name in other_intlookup_names:
            other_intlookup = options.db.intlookups.get_intlookup(other_intlookup_name)
            if sum(map(lambda x: len(x.brigades), other_intlookup.brigade_groups)) > 0:
                raise Exception("Can not generate new trivial intlookup while intlookup %s exists/not empty" % other_intlookup_name)

    return options


def main(options):
    N, tiers = options.db.tiers.primus_int_count(options.shard_count)

    if N % options.bases_per_group:
        raise Exception('Total number of shards <{}> is not divied by number of shards in intgroup <{}>'.format(N, options.bases_per_group))

    if options.keep_existing_intlookups:
        instances = reduce(lambda x, y: x + y,
                           map(lambda x: list(set(x.get_instances()) - set(x.get_busy_instances())), options.groups),
                           [])
    else:
        instances = reduce(lambda x, y: x + y, map(lambda x: x.get_instances(), options.groups), [])

    instances = filter(options.instance_filter, instances)
    if options.shuffle:
        random.shuffle(instances)
    elif options.uniform_by_host:
        instances.sort(cmp=lambda x, y: cmp((x.port, x.host.name), (y.port, y.host.name)))
    else:
        instances.sort(cmp=lambda x, y: cmp('%s:%s:%s' % (x.host.dc, x.host.name, x.port),
                                            '%s:%s:%s' % (y.host.dc, y.host.name, y.port)))

    # check if we have enough replicas
    if options.min_replicas is not None and len(
            instances) < N * options.min_replicas and not options.create_empty_intlookup:
        raise Exception("Not enough instances: have %s, needed %s" % (len(instances), N * options.min_replicas))

    if options.db.intlookups.has_intlookup(options.output_file):
        options.db.intlookups.remove_intlookup(options.output_file)
    intlookup = options.db.intlookups.create_empty_intlookup(options.output_file)
    intlookup.brigade_groups_count = N / options.bases_per_group
    intlookup.tiers = tiers
    intlookup.hosts_per_group = options.bases_per_group
    intlookup.base_type = options.groups[0].card.name  # FIXME
    intlookup.intl2_groups.append(TIntl2Group())

    if not options.create_empty_intlookup:
        multishard_groups = map(lambda x: TMultishardGroup(), xrange(intlookup.brigade_groups_count))
        i = 0
        while len(instances) >= intlookup.hosts_per_group:
            if options.max_replicas is not None and i == 0 and len(
                    multishard_groups[0].brigades) == options.max_replicas:
                break

            brigade_instances = map(lambda x: [x], instances[:intlookup.hosts_per_group])
            brigade = TIntGroup(brigade_instances, [])
            multishard_groups[i].brigades.append(brigade)
            instances = instances[intlookup.hosts_per_group:]
            i = (i + 1) % intlookup.brigade_groups_count

        intlookup.intl2_groups[0].multishards = multishard_groups

    # mark modified
    intlookup.mark_as_modified()
    for group in options.groups:
        group.mark_as_modified()

    if not options.no_apply:
        options.db.intlookups.update(smart=True)
        options.db.groups.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
