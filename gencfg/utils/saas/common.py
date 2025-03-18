# coding: utf-8

"""
Common SaaS functions and constants
"""
import re
import math
import argparse
from core.card.node import CardNode
from gaux.aux_volumes import TVolumeInfo

try:
    from typing import List
except ImportError:
    pass

import gencfg
from core.db import CURDB

from utils.saas.saas_byte_size import SaaSByteSize

IGNORE_NOINDEX_TAG_ITYPES = ('searchproxy',)
IGNORE_NETWORK_CRITICAL_LOCATIONS = frozenset({'MAN', 'VLA', })
ALL_GEO = frozenset({'SAS', 'MAN', 'VLA', 'IVA'})
DEFAULT_IO_LIMITS = {'ssd': {'read': 25, 'write': 20}, 'hdd': {'read': 10, 'write': 10}}


class VolumesOptions(object):
    def __init__(self, groups, vs):
        self.json_volumes = vs
        self.groups = [CURDB.groups.get_group(group) for group in groups]


def get_system_used_power(host):
    system_used_cores = 4.0
    power_per_core = host.power/host.ncpu
    return int(math.ceil(power_per_core * system_used_cores))


def gencfg_io_limits_from_json(io_limits_json):
    def extended_io_limit(compact_io_limits):
        ops_limits = {}
        for k, v in compact_io_limits.items():
            if k == 'ssd':
                ops_limits[k] = {key: value * 256 for key, value in v.items()}  # 4k blocks
            elif k == 'hdd':
                ops_limits[k] = {key: value * 16 for key, value in v.items()}  # 16k blocks
        return {
            'bandwidth': compact_io_limits, 'ops': ops_limits
        }

    gencfg_io_limits = {}
    extended_io_limits_json = extended_io_limit(io_limits_json)
    for limit_type in ('bandwidth', 'ops'):
        if limit_type == 'bandwidth':
            limit_alias = 'io'

            def value_processor(x):
                return SaaSByteSize('{}MB'.format(x))
        else:
            limit_alias = 'io_ops'

            def value_processor(x):
                return x

        limits = extended_io_limits_json.get(limit_type, None)
        if limits is not None:
            for device in ('ssd', 'hdd'):
                device_limits = limits.get(device, None)
                if device_limits is not None:
                    gencfg_io_limits.update({('reqs', 'instances', '{}_{}_{}_limit'.format(device, limit_alias, direction)): value_processor(value) for direction, value in device_limits.items()})
    return gencfg_io_limits


def get_group_allocated_volume(group, mount_point='/ssd'):
    """
    Calculate size of allocated volumes by mount point
    :param group: Gencfg group
    :type group: core.igroups.IGroup
    :param mount_point: mount point
    :type mount_point: str
    :rtype: SaaSByteSize
    """
    return get_allocated_volume(group.card.reqs.volumes, mount_point=mount_point)


def get_allocated_volume(volumes, mount_point='/ssd'):
    result = SaaSByteSize.from_int_bytes(0)
    for volume in volumes:
        if isinstance(volume, CardNode):
            volume = volume.as_dict()
        elif isinstance(volume, TVolumeInfo):
            volume = volume.to_json()
        if volume['host_mp_root'] == mount_point:
            result += SaaSByteSize(volume['quota'])

    return result


def merge_volumes(volumes, volumes_update):
    default_volume_template = {'guest_mp': None, 'host_mp_root': '/place', 'quota': '1 Gb', 'symlinks': []}
    volumes = {vol['guest_mp']: vol.as_dict() for vol in volumes}
    if isinstance(volumes_update, list):
        volumes_update = {vol[['guest_mp']]: vol for vol in volumes}

    for mount_point, volume_update in volumes_update.items():
        volume_update['quota'] = SaaSByteSize(volume_update.get('quota', '1 Gb'))
        merge_candidate = volumes.get(mount_point, default_volume_template)
        merge_candidate.update(volume_update)

    return [v for v in volumes.values()]


def is_saas_group(group):
    master_group = group.card.master
    return bool(master_group and re.match('^({})_SAAS_CLOUD$'.format('|'.join(ALL_GEO)), master_group.card.name) and 'BLACKLISTED' not in group.card.name)


def get_noindexing(group):
    if is_saas_group(group) and group.card.tags.itype not in IGNORE_NOINDEX_TAG_ITYPES:
        return ('saas_no_indexing' in group.card.tags.itag) or (group.card.reqs.instances.affinity_category == 'saas_no_indexing')
    else:
        return None


def get_network_critical(group):
    if is_saas_group(group):
        return 'saas_network_critical' in group.card.tags.itag
    else:
        return None


def instances_per_host(group):
    """
    :param group: group to check
    :type group: core.igroups.IGroup
    :return: How many instances group want per host
    :rtype: int
    """
    if group.card.legacy.funcs.instanceCount and group.card.legacy.funcs.instanceCount != 'default':
        if group.card.legacy.funcs.instanceCount.startswith('exactly'):
            iph = int(group.card.legacy.funcs.instanceCount[7:])
        else:
            raise RuntimeError('Unknown instance count function "{}"'.format(group.card.legacy.funcs.instanceCount))
    else:
        iph = 1
    return iph


def get_group_instance_power(group):
    if group.card.legacy.funcs.instancePower is not None:
        instance_power_match = re.match(r'exactly(\d+)', group.card.legacy.funcs.instancePower)
        if instance_power_match:
            return int(instance_power_match.group(1))
        else:
            raise ValueError('Unknown instance power function')
    elif group.card.reqs.instances.power > 1:
        return group.card.reqs.instances.power
    elif group.card.reqs.shards.min_power > 1:
        return group.card.reqs.shards.min_power
    else:
        raise ValueError('Can\'t compute group instance power')


class NumberWithSign(object):
    def __init__(self, s):
        if s.startswith('+'):
            self.sign = '+'
        elif s.startswith('-'):
            self.sign = '-'
        else:
            self.sign = '='
            s = s.lstrip('=')
        self.value = int(s, 10)

    def __mul__(self, other):
        if isinstance(other, int):
            return NumberWithSign('{}{}'.format(self.sign, self.value * other))
        else:
            raise TypeError('NumberWithSign only support multiplication by int')

    def __nonzero__(self):
        return self.value != 0


def number_with_sign(string):
    try:
        return NumberWithSign(string)
    except ValueError:
        raise argparse.ArgumentTypeError('Only base 10 integers with optional sign allowed')
