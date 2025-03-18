#!/skynet/python/bin/python
"""Find hosts with low cpu/mem usage for using them in background groups (GENCFG-649)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from utils.pregen import find_most_memory_free_machines
from utils.clickhouse import show_unused


def get_parser():
    parser = ArgumentParserExt(description="Find machines with low usage of cpu and enough memory")
    parser.add_argument("-s", "--src-groups", type=argparse_types.groups, required=True,
            help="Obligatory. List of groups to search unused hosts from")
    parser.add_argument("--max-cpu-usage", type=float, required=True,
            help="Obligatory. Filter out hosts with max usage more than specified (1.0 means 100%% usage)")
    parser.add_argument("--min-memory-free", type=float, required=True,
            help="Obligatory. Filter out host with free memory more than specified (in Gb)")
    parser.add_argument("--add-to-group", type=argparse_types.group, default=None,
            help="Optional. Group to add hosts to (if not specified do nothing)")

    return parser


def main(options):
    candidate_hosts = set(sum([x.getHosts() for x in options.src_groups], []))
    print 'Src groups: {} hosts'.format(len(candidate_hosts))

    # filter out groups without allow_background_groups
    no_background_hosts = set()
    for group in CURDB.groups.get_groups():
        if group.card.properties.allow_background_groups == False:
            no_background_hosts |= set(group.getHosts())
    candidate_hosts -= no_background_hosts
    print 'After removing no_background groups: {} hosts'.format(len(candidate_hosts))

    # filter out hosts with not enough memory
    util_params = dict(
        action='show',
        hosts=list(candidate_hosts),
    )
    result = find_most_memory_free_machines.jsmain(util_params)
    candidate_hosts = {x.host for x in result if x.memory_left > options.min_memory_free}
    print 'After removing hosts with not enough memory: {} hosts'.format(len(candidate_hosts))

    # filter out hosts with not enough cpu
    util_params = dict(
        hosts=list(candidate_hosts),
        usage_ratio=options.max_cpu_usage
    )
    result = show_unused.jsmain(util_params)
    candidate_hosts = {str(x[0]) for x in result[0].hosts_with_usage}
    candidate_hosts = {CURDB.hosts.get_host_by_name(x) for x in candidate_hosts}
    print 'After removing hosts with high cpu usage: {} hosts'.format(len(candidate_hosts))

    if options.add_to_group:
        candidate_hosts -= set(options.add_to_group.getHosts())
        print 'After removing hosts already present in {}: {} hosts'.format(options.add_to_group.card.name, len(candidate_hosts))
        for host in candidate_hosts:
            if not options.add_to_group.hasHost(host):
                options.add_to_group.addHost(host)
        CURDB.update(smart=True)
        print 'Added {} hosts to group {}'.format(len(candidate_hosts), options.add_to_group.card.name)

    return list(candidate_hosts)


def print_result(options, result):
    print 'Found {} hosts with usage less than <{}> and free memory more than <{}>: {}'.format(
            len(result), options.max_cpu_usage, options.min_memory_free, ' '.join(x.name for x in result[:100]))


def jsmain(d):
    options = get_parser().parse_json(d)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    result = main(options)

    print_result(options, result)
