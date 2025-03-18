#!/skynet/python/bin/python

import os
import sys
import json
import argparse
import logging

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types

from utils.saas.saas_byte_size import SaaSByteSize
from utils.saas.saas_host_info import SaaSHostInfo
from utils.saas.saas_sort import sort_and_filter_by_switch
from utils.saas.common import IGNORE_NETWORK_CRITICAL_LOCATIONS


def get_parser():
    parser = ArgumentParserExt(
        description="Find machines with desired properties in SaaS cloud")
    parser.add_argument('-g', '--group', type=argparse_types.group, default=None, required=True,
                        help='Obligatory. Group to search machines in')
    parser.add_argument('-x', '--exclude-groups', type=argparse_types.groups, default=[],
                        help='Optional. List of groups. Hosts from this groups will not be used')
    parser.add_argument('-c', '--slot-size-cpu', type=float, required=True,
                        help='Obligatory. Target slot CPU in Kimkim power units (1 core ~= 40 Pu)')
    parser.add_argument('-m', '--slot-size-mem', type=float, required=True,
                        help='Obligatory. Target slot memory Gb')
    parser.add_argument('-ssd', '--slot-size-ssd', type=float, required=True,
                        help='Obligatory. Target slot SSD Gb')
    parser.add_argument('-i', '--no-indexing', action='store_true')
    parser.add_argument('-f', '--file', type=argparse.FileType('w'), default=sys.stdout,
                        help='File for result json')
    return parser


def get_host_info(group, req_memory, req_cpu, req_ssd, no_indexing, network_critical,
                  exclude_groups=None, max_hosts_per_switch=0, target_group=None, prefer_empty=False, strict=False):
    """
    Collect info about hosts in group
    :param group: Group to find hosts in
    :param req_memory: group memory requirements
    :param req_cpu: group cpu requirements in Kimkim power units
    :param req_ssd: group ssd requirements
    :param exclude_groups: list of groups, hosts present in this groups will be filtered from result
    :type group: core.igroups.IGroup
    :type req_cpu: int
    :type req_memory: SaaSByteSize
    :type req_ssd: SaaSByteSize
    :type exclude_groups: set[core.igroups.IGroup]
    :return: sorted list of suitable hosts
    :rtype: list[utils.saas.common.SaaSHostInfo]
    """
    hosts_to_process = group.getHosts()
    if not (group and req_memory and req_cpu):
        raise RuntimeError('Can\'t collect hosts without group or basic requirements')

    req_ssd = req_ssd if req_ssd else SaaSByteSize.from_int_bytes(0)

    # add info on candidate hosts
    if exclude_groups:
        excluded_hosts = set(sum(map(lambda x: x.getHosts(), exclude_groups), []))
    else:
        excluded_hosts = set()

    filtered_hosts = list(set(hosts_to_process) - excluded_hosts)
    logging.info('%d hosts excluded due to been in blacklisted groups. %d left for filtering', len(excluded_hosts), len(filtered_hosts))

    hosts_info = [SaaSHostInfo(h) for h in filtered_hosts]

    hosts_with_good_net = set(filter(lambda host: (host.net >= 10000 and len(host.network_critical_groups) == 0 if network_critical else host.net < 10000) or (host.host.dc.upper() in IGNORE_NETWORK_CRITICAL_LOCATIONS), hosts_info))
    logging.info('%d hosts have suitable network', len(hosts_with_good_net))

    hosts_with_enough_cpu = set(filter(lambda host: host.cpu_left >= req_cpu, hosts_info))
    logging.info('%d hosts match by spare cpu power', len(hosts_with_enough_cpu))

    hosts_with_enough_mem = set(filter(lambda host: host.memory_left >= req_memory, hosts_info))
    logging.info('%d hosts match by spare memory', len(hosts_with_enough_mem))

    hosts_with_enough_ssd = set(filter(lambda host: host.ssd_left >= req_ssd, hosts_info))
    logging.info('%d hosts match by spare ssd volume', len(hosts_with_enough_ssd))

    hosts_info = hosts_with_enough_cpu.intersection(hosts_with_enough_mem).intersection(hosts_with_enough_ssd)

    if strict:
        logging.info('Strict filtering enabled: filter by network')
        hosts_info = hosts_info.intersection(hosts_with_good_net)

    logging.info('%d hosts fits all resource requirements', len(hosts_info))

    max_hosts_per_switch = max_hosts_per_switch if max_hosts_per_switch else len(hosts_info)
    logging.info('Sorting filtered hosts')
    return sort_and_filter_by_switch(
        list(hosts_info), req_memory, req_cpu, req_ssd, network_critical,
        max_hosts_per_switch=max_hosts_per_switch, target_group=target_group,
    )


def main(group, slot_size_mem, slot_size_cpu, slot_size_ssd, no_indexing=False, exclude_groups=None):
    host_info = get_host_info(group, req_memory=slot_size_mem, req_cpu=slot_size_cpu, req_ssd=slot_size_ssd,
                              no_indexing=no_indexing, exclude_groups=exclude_groups)
    return json.dumps([h.to_dict(measurement_unit='Gb') for h in host_info])


if __name__ == '__main__':
    args = get_parser().parse_cmd()

    result = main(args.group,
                  slot_size_cpu=args.slot_size_cpu,
                  slot_size_mem=SaaSByteSize('{} Gb'.format(args.slot_size_mem)),
                  slot_size_ssd=SaaSByteSize('{} Gb'.format(args.slot_size_ssd)),
                  no_indexing=args.no_indexing, exclude_groups=args.exclude_groups)
    if args.file:
        args.file.write(result)
    else:
        sys.stdout.write(result)
