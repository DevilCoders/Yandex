#!/skynet/python/bin/python
"""Move background groups from hosts, where background groups are not allowed

Groups like SAS_MAIL_LUCENE MSK_ST_TASK_GENCFG_697_BASE work in background with any group. Unfortunately,
not every group can live with these groups"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from utils.pregen import find_most_memory_free_machines
from utils.clickhouse import show_unused

def get_parser():
    parser = ArgumentParserExt(usage="Move background groups from hosts where background groups are not welcome")
    parser.add_argument('-g', '--groups', type=argparse_types.groups, required=True,
                        help="Obligatory. List of background group to process")
    parser.add_argument('-f', '--candidates-filter', type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on candidates for replacement")
    parser.add_argument('-x', '--extra-forbid-hosts', type=argparse_types.hosts, default=[],
                        help='Optional. Extra hosts to remove from background group')
    parser.add_argument('--extra-forbid-groups', type=argparse_types.groups, default=[],
                        help='Optional. Extra groups to exclude')
    parser.add_argument('--add-extra', type=int, default=0,
                        help='Optional. Add extra instances')

    return parser


def find_candidates(options, group):
    # find all used hosts
    used_hosts = set(group.getHosts())

    # find not welcome hosts
    forbid_hosts = [list(x.get_kinda_busy_hosts()) for x in CURDB.groups.get_groups() if x.card.properties.allow_background_groups == False]
    forbid_hosts = set(sum(forbid_hosts, []))
    forbid_hosts |= set(options.extra_forbid_hosts)
    forbid_hosts |= set(sum([list(x.getHosts()) for x in options.extra_forbid_groups], []))
    forbid_hosts |= {x for x in used_hosts if not options.candidates_filter(x)}

    # find candidates
    print '    Search for candidates:'
    candidate_hosts = CURDB.groups.get_group('ALL_SEARCH').getHosts()
    print '        Candidates total: {}'.format(len(candidate_hosts))

    # filter candidates by removing forbid hosts
    candidate_hosts = [x for x in candidate_hosts if x not in forbid_hosts]
    print '        Candidates after removing forbid hosts: {}'.format(len(candidate_hosts))

    # filter candidates by user filter
    candidate_hosts = [x for x in candidate_hosts if options.candidates_filter(x)]
    print '        Candidates after applying user filter: {}'.format(len(candidate_hosts))

    # filter candidates by location
    if group.card.reqs.hosts.location.location != []:
        allow_locations = group.card.reqs.hosts.location.location
        candidate_hosts = [x for x in candidate_hosts if x.location in allow_locations]
    print '        Candidates after filter by location: {}'.format(len(candidate_hosts))

    # filter candidates, removing hosts already in group
    candidate_hosts = [x for x in candidate_hosts if x not in used_hosts]
    print '        Candidates after removing already used hosts: {}'.format(len(candidate_hosts))

    # filter candidates by memory
    iperhost = 1
    m = re.match('exactly(\d+)', group.card.legacy.funcs.instanceCount)
    if m:
        iperhost = int(m.group(1))
    need_memory = group.card.reqs.instances.memory_guarantee.gigabytes() * iperhost + 1
    need_cpu = group.get_instances()[0].power * iperhost

    util_params = dict(
        action='show',
        hosts=candidate_hosts,
    )
    result = find_most_memory_free_machines.jsmain(util_params)

    ignore_cpu_groups = ['MAN_WEB_BASE', 'SAS_WEB_BASE', 'VLA_WEB_BASE', 'VLA_YT_RTC', 'MAN_IMGS_BASE', 'SAS_IMGS_BASE',
                         'MAN_VIDEO_BASE']
    ignore_cpu_groups = [CURDB.groups.get_group(x) for x in ignore_cpu_groups]
    ignore_cpu_hosts = set(sum([x.getHosts() for x in ignore_cpu_groups], []))

    candidate_hosts = [x.host for x in result if x.memory_left >= need_memory and ((x.cpu_left >= need_cpu) or (x.host in ignore_cpu_hosts))]

    print '        Candidates with at least {:.2f} free memory and {:.2f} cpu: {}'.format(need_memory, need_cpu, len(candidate_hosts))

    return candidate_hosts


def main_for_group(options, group):
    """Remove hosts from specified backgroun groups"""

    print 'Processing {}:'.format(group.card.name)

    used_hosts = set(group.getHosts())

    # find not welcome hosts
    forbid_hosts = [list(x.get_kinda_busy_hosts()) for x in CURDB.groups.get_groups() if (x.card.properties.allow_background_groups == False) or (x.card.properties.nonsearch == True)]
    forbid_hosts = set(sum(forbid_hosts, []))
    forbid_hosts |= set(options.extra_forbid_hosts)
    forbid_hosts |= {x for x in used_hosts if not options.candidates_filter(x)}
    remove_hosts = used_hosts & forbid_hosts
    print '    Need to remove {} hosts: {}'.format(len(remove_hosts), ' '.join((x.name for x in remove_hosts)))

    need_hosts = len(remove_hosts) + options.add_extra

    if need_hosts == 0:
        return

    # find candidates
    candidate_hosts = find_candidates(options, group)
    if len(candidate_hosts) < need_hosts:
        raise Exception('Found only {} candidates when need to replace {} hosts in group {}'.format(len(candidate_hosts), need_hosts, group.card.name))

    # find most unused
    util_params = dict(
        hosts=candidate_hosts,
        topn=need_hosts
    )
    result = show_unused.jsmain(util_params)
    max_usage = result[0].hosts_with_usage[-1][1]
    add_hosts = [CURDB.hosts.get_host_by_name(str(x[0])) for x in result[0].hosts_with_usage]

    if len(add_hosts) < need_hosts:
        raise Exception('Not enough hosts with statistics in show_unused (only {} of {})'.format(len(add_hosts), len(candidate_hosts)))

    print '    Replacement hosts ({:.2f} max usage): {}'.format(max_usage, ' '.join(x.name for x in add_hosts))

    # remove old hosts, add new hosts
    CURDB.groups.remove_slave_hosts(list(remove_hosts), group)
    CURDB.groups.add_hosts(list(add_hosts), group)


def main(options):
    for group in options.groups:
        main_for_group(options, group)

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
