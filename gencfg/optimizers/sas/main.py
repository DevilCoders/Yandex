#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types

from groupsinfo import load_hgroups
from shardsinfo import load_tiers, check_and_add_replicas, show_shards
from genoutput import generate_intlookups
from optimizer import optimize, optimize_ssd


def parse_cmd():
    parser = ArgumentParser(description="Create intlookup using sas optimization algorithm")

    parser.add_argument("-g", "--group-name", dest="group_name", type=str, default=None, required=True,
                        help="Obligatory. Group name")
    parser.add_argument("-t", "--tiers-data", dest="tiers_data", type=argparse_types.sasconfig, default=None,
                        required=True,
                        help="Obligatory. File with replicas and power data")
    parser.add_argument("-o", "--hosts-per-group", dest="hosts_per_group", type=int, default=None, required=True,
                        help="Obligatory. Hosts per group")
    parser.add_argument("-s", "--optimize-ssd", action="store_true", dest="optimize_ssd", default=False,
                        help="Optional. Perform ssd optimization")
    parser.add_argument("-e", "--extra-replicas-distribution-mode", type=str, dest="extra_replicas_distribution_mode",
                        default='max',
                        choices=['max', 'min', 'uniform', 'fair'],
                        help="Optional. How we distribute extra replicas")
    parser.add_argument("-f", "--filters", action="append", type=str,
                        help="Options. Filter for hosts")
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose", default=False,
                        help="Optional. Make output vebose")
    parser.add_argument("-l", "--solution-filter", dest="solution_filter", type=str, default=None,
                        help="Optional. Extra solution filter (e.g. 'lambda hgroup, shards: hgroup.ssd is False or len(filter(lambda x: x.tier_name == 'RRGTier0', shards)) >= 1')")
    parser.add_argument('--exclude-cpu-from', type=argparse_types.groups, default = (),
                        help='Optional. Do not distribute cpu from specified groups (RX-400)')
    parser.add_argument('--exclude-hosts-from', type=argparse_types.groups, default = (),
                        help='Optional. Do not distribute hosts from specified groups')
    parser.add_argument('--dry-run', action="store_true", dest="dry_run", default=False,
                        help='Optional. Run optimization but do not write any data back in the database')

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.filters is None:
        options.filters = [lambda x: True]
    else:
        options.filters = map(lambda x: eval(x), options.filters)
    for host in CURDB.groups.get_group(options.group_name).getHosts():
        if not len(filter(lambda x: x(host), options.filters)):
            raise Exception("Host %s does not satisfy any filter" % host.name)
    if options.solution_filter:
        options.solution_filter = eval(options.solution_filter)

    return options


def optimize_for_flt(options, flt):
    hgroups = load_hgroups(options.group_name, options.hosts_per_group, separate_ssd=options.optimize_ssd,
                           flt=flt, verbose=options.verbose, exclude_cpu_from=options.exclude_cpu_from, exclude_hosts_from=options.exclude_hosts_from)

    tiers = load_tiers(options.tiers_data, options.hosts_per_group)
    check_and_add_replicas(tiers, hgroups, options.extra_replicas_distribution_mode, options.optimize_ssd)

    shards = sum(map(lambda x: x.shards, tiers), [])
    print shards
    optimize(hgroups, shards, options.solution_filter)
    if options.optimize_ssd:
        optimize_ssd(shards)

    return hgroups, tiers


if __name__ == '__main__':
    options = parse_cmd()

    hgroups, tiers = optimize_for_flt(options, options.filters[0])
    for flt in options.filters[1:]:
        cur_hgroups, cur_tiers = optimize_for_flt(options, flt)
        hgroups += cur_hgroups
        for i in range(len(tiers)):
            tiers[i].append_other(cur_tiers[i])

    show_shards(tiers, options.verbose)
    if options.dry_run:
        print 'DRY RUN, intlookups and groups left unchanged'
    else:
        generate_intlookups(tiers, hgroups, options.group_name)
        CURDB.update(smart=True)
