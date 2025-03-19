# -*- coding: utf-8 -*-
"""
Utilities specific for SQL Server clusters.
"""
from typing import Tuple


from ..utils import build_host_group, combine_sg_service_rules, HostGroup
from ...utils import get_first_value

DE_ROLE_TYPE = 'sqlserver_cluster'
WITNESS_ROLE_TYPE = 'windows_witness'


def classify_host_map(hosts: dict) -> Tuple[dict, dict]:
    """
    Classify dict of hosts on Database Engine and Witness ones.
    """
    de_hosts = {}
    witness_hosts = {}
    for host, opts in hosts.items():
        if WITNESS_ROLE_TYPE in opts.get('roles', []):
            witness_hosts[host] = opts
        else:
            de_hosts[host] = opts

    return de_hosts, witness_hosts


def sqlserver_build_host_groups(hosts: dict, config) -> Tuple[HostGroup, HostGroup]:
    """
    Get Database Engine and Witness hosts groups.
    """
    de_hosts, witness_hosts = classify_host_map(hosts)

    de_group = build_host_group(config.sqlserver, de_hosts)
    if not de_group.properties:
        raise RuntimeError('Configuration error: has de_hosts but not de_group.properties')
    assert de_group.properties
    witness_group = build_host_group(config.windows_witness, witness_hosts)

    if witness_hosts:
        if not witness_group.properties:
            raise RuntimeError('Configuration error: has witness_hosts but not witness_group.properties')
        witness_group.properties.conductor_group_id = get_first_value(witness_group.hosts)['subcid']

        # Only one service SG per cluster is supported for now. So each host group should have rules
        # combined from all groups.
        sg_service_rules = combine_sg_service_rules(de_group, witness_group)
        de_group.properties.sg_service_rules = sg_service_rules
        witness_group.properties.sg_service_rules = sg_service_rules
        for fqdn, opts in witness_group.hosts.items():
            opts['host_group_ids'] = []

    return de_group, witness_group
