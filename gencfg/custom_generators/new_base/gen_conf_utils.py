# -*- coding: utf-8 -*-
import os
import sys
import collections

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

import gencfg  # noqa
import gaux.aux_shards  # noqa
import gaux.aux_hbf  # noqa
from core.db import CURDB  # noqa


def config_name(hostname, port, workload_name=None):
    if workload_name:
        return '{host}:{port}_{workload}_embeddingstorage.cfg.proto'.format(
            host=(hostname if 'gencfg-c' in hostname else short_name(hostname)),
            port=port,
            workload=workload_name,
        )
    return '{host}:{port}_embeddingstorage.cfg.proto'.format(
        host=(hostname if 'gencfg-c' in hostname else short_name(hostname)),
        port=port,
    )


def short_name(hostname):
    return hostname.split('.')[0]


def make_mtn_instance(group, instance):
    host = gaux.aux_hbf.generate_mtn_hostname(instance, group, '')
    return host, instance.port


def instance_to_shard_mapping(group_name, workload_name=None):
    group = CURDB.groups.get_group(group_name)
    intlookup = CURDB.intlookups.get_intlookup(group.card.intlookups[0])

    mapping = {}
    for shard_id in xrange(intlookup.get_shards_count()):
        shard_name = intlookup.get_primus_for_shard(shard_id, for_searcherlookup=False)
        for instance in intlookup.get_base_instances_for_shard(shard_id):
            if group.card.properties.mtn.use_mtn_in_config:
                instance = make_mtn_instance(group, instance)

            if workload_name:
                instance = instance[0], instance[1] + get_workload_port_offset(group, workload_name)
            mapping[instance] = shard_name
    return mapping


def split_mapping_by_partitions(mapping):
    """split instance_to_shard_mapping by partitions"""
    result = collections.defaultdict(dict)
    for host_port, shard_template in mapping.items():
        partition = gaux.aux_shards.partition_number(shard_template)
        result[partition][host_port] = shard_template
    return dict(result)


def invert_mapping(mapping):
    result = collections.defaultdict(list)
    for key, value in mapping.items():
        result[value].append(key)
    return dict(result)


def store_cfg(target_dir, cfg_name, content):
    with open(os.path.join(target_dir, cfg_name), 'w') as f:
        f.write(content)


def get_group_workloads_names(group_name):
    group = CURDB.groups.get_group(group_name)

    if 'workloads' in group.card:
        return {workload.name for workload in group.card.workloads}
    return set()


def get_workload_port_offset(group, workload_name):
    for workload in group.card.workloads:
        if workload.name == workload_name:
            return workload.port_offset
    raise RuntimeError('Workload %s not found in group %s', workload_name, group.card.name)
