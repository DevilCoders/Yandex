# coding: utf-8

import math
import operator
from itertools import groupby
from collections import defaultdict
import logging
try:
    from typing import List
except ImportError:
    pass
import gencfg
from core.db import CURDB
from utils.saas.saas_host_info import SaaSHostInfo
from utils.saas.saas_byte_size import SaaSByteSize
from utils.saas.common import ALL_GEO

RESOURCE_SCORES = {
    geo: {'mem': 0, 'ssd': 0}
    for geo in ALL_GEO
}

for geo in ALL_GEO:
    left_cpu = 1
    left_ram = SaaSByteSize.from_int_bytes(1)
    left_ssd = SaaSByteSize.from_int_bytes(1)

    master_group = CURDB.groups.get_group('{}_SAAS_CLOUD'.format(geo))
    hosts = [SaaSHostInfo(h) for h in master_group.getHosts()]
    for host in hosts:
        host_cpu = host.cpu_left if host.cpu_left > 0 else 0
        left_cpu += host_cpu
        left_ram += host.memory_left
        left_ssd += host.ssd_left

    RESOURCE_SCORES[geo]['mem'] = left_cpu/float(left_ram.value)
    RESOURCE_SCORES[geo]['ssd'] = left_cpu/float(left_ssd.value)


def sort_and_filter_by_switch(hosts_to_filter, req_memory, req_cpu, req_ssd, network_critical,
                              max_hosts_per_switch=1, target_group=None, prefer_empty=True):
    """
    Default hosts sort function. It determines the list of suitable hosts depending on the
    requirements.
    CPU is considered in kimkim units.
    :param hosts_to_filter: List of hosts to sort
    :param req_memory: group memory requirements
    :param req_cpu: type int group CPU requirements in Kimkim power units ~40 per core
    :type hosts: list[SaaSHostInfo]
    :type req_memory: SaaSByteSize
    :type req_cpu: int
    :type max_hosts_per_switch: int
    :type target_group: core.igroups.IGroup
    :rtype list[SaaSHostInfo]
    """
    host_switch = operator.attrgetter('switch')
    host_net = operator.attrgetter('net')
    result_hosts = []
    filtered_hosts =[]
    current_hosts_by_switch = defaultdict(int)

    if target_group is not None:
        target_group_hosts = map(lambda x: SaaSHostInfo(x), target_group.getHosts())
        target_group_hosts.sort(key=host_switch)
        for k, g in groupby(target_group_hosts, host_switch):
            current_hosts_by_switch[k] += len(list(g))

    def host_score(ho):
        g_scores = RESOURCE_SCORES[ho.host.dc.upper()]

        l_cpu = (ho.cpu_left - req_cpu)
        l_mem = (ho.memory_left - req_memory).value * g_scores['mem']
        l_ssd = (ho.ssd_left - req_ssd).value * g_scores['ssd']

        norm = math.sqrt(3 * (l_cpu**2 + l_mem**2 + l_ssd**2))
        cos = abs((l_cpu + l_mem + l_ssd)/norm)

        if l_cpu < 0 or l_mem < 0 or l_ssd < 0:
            logging.info('Calculated %s vector c: %d, m: %f, s:%f;cos:%f', ho.host.name, l_cpu, l_mem, l_ssd, cos)
        else:
            logging.debug('Calculated %s vector c: %d, m: %f, s:%f;cos:%f', ho.host.name, l_cpu, l_mem, l_ssd, cos)

        cos_score = 1 - cos
        return cos_score

    hosts_to_filter.sort(key=host_switch)
    for k, g in groupby(hosts_to_filter, host_switch):
        if current_hosts_by_switch[k] < max_hosts_per_switch or not max_hosts_per_switch:
            filtered_hosts.extend(sorted(g, key=host_score, reverse=True)[:(max_hosts_per_switch - current_hosts_by_switch[k])])

    filtered_hosts.sort(key=host_net, reverse=True)
    filtered_hosts_by_net = {k: list(g) for k, g in groupby(filtered_hosts, host_net)}
    for net in sorted(filtered_hosts_by_net.keys(), reverse=network_critical):
        result_hosts.extend(sorted(filtered_hosts_by_net[net], reverse=True))
    return result_hosts
