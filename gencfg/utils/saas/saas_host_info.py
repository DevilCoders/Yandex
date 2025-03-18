# coding: utf-8

import operator
import math
try:
    from typing import List
except ImportError:
    pass

import gencfg
from core.db import CURDB
from utils.saas.saas_byte_size import SaaSByteSize
from utils.saas.common import is_saas_group, get_noindexing, get_network_critical, get_system_used_power
from utils.saas.common import get_group_allocated_volume


class SaaSHostInfo(object):
    IGNORE_NOINDEX_LOCATIONS = ('VLA', 'MAN', 'SAS', 'IVA')

    class SaaSInstanceAssignedResources(object):

        @staticmethod
        def _io_limit_attrs(t='bytes'):
            t_str = '{}_io_{}_limit' if t == 'bytes' else '{}_io_ops_{}_limit'
            result = []
            for action in {'read', 'write'}:
                for storage in {'ssd', 'hdd'}:
                    result.append(t_str.format(storage, action))
            return result

        def __init__(self, instance, group):
            """
            :type instance: core.instances.Instance
            :param group: core.igroups.IGroup
            """
            self.instance = instance
            self.cpu = int(math.ceil(instance.power))
            self.cpu_class = group.card.audit.cpu.class_name
            self.cpu_limit = int(group.card.audit.cpu.greedy_limit or self.cpu)
            self.memory = SaaSByteSize(group.card.reqs.instances.memory_guarantee)
            self.memory_limit = self.memory + SaaSByteSize(group.card.reqs.instances.memory_overcommit)
            self.volumes = group.card.reqs.volumes
            self.ssd = get_group_allocated_volume(group, '/ssd')
            self.hdd = get_group_allocated_volume(group, '/place')
            self.is_saas = is_saas_group(group)
            self.noindex = get_noindexing(group)
            self.network_critical = get_network_critical(group)
            for io_limit_attr in self._io_limit_attrs('bytes'):
                setattr(self, io_limit_attr, SaaSByteSize(getattr(group.card.reqs.instances, io_limit_attr)))
            for io_limit_attr in self._io_limit_attrs('ops'):
                setattr(self, io_limit_attr, getattr(group.card.reqs.instances, io_limit_attr))

        def __repr__(self):
            return '{type}: cpu:{cpu} mem:{mem} ssd:{ssd} noindex:{noindex}'.format(
                type=self.instance.type, cpu=self.cpu, mem=self.memory, ssd=self.ssd, noindex=self.noindex
            )

        @property
        def name(self):
            return self.instance.type

        def io_limits_serializable(self, unit=None):
            unit_attr = 'in_{}'.format(unit) if unit else 'text'
            io_limits = {
                attr: getattr(getattr(self, attr), unit_attr) for attr in self._io_limit_attrs('bytes')
            }
            io_limits_ops = {
                attr: getattr(self, attr) for attr in self._io_limit_attrs('ops')
            }
            io_limits.update(io_limits_ops)
            return io_limits

        def to_dict(self, unit=None):
            if unit:
                mem = getattr(self.memory, 'in_{}'.format(unit))
                mem_limit = getattr(self.memory_limit, 'in_{}'.format(unit))
                ssd = getattr(self.ssd, 'in_{}'.format(unit))
                hdd = getattr(self.hdd, 'in_{}'.format(unit))
            else:
                mem = str(self.memory)
                mem_limit = str(self.memory_limit)
                ssd = str(self.ssd)
                hdd = str(self.hdd)
            base_dict = {
                'group': self.instance.type,
                'is_saas_group': self.is_saas,
                'cpu_class': self.cpu_class,
                'cpu_guarantee': self.cpu,
                'cpu_limit': self.cpu_limit,
                'memory_guarantee': mem,
                'memory_limit': mem_limit,
                'allocated_ssd': ssd,
                'allocated_hdd': hdd,
                'noindexing': self.noindex,
                'io_limits': self.io_limits_serializable(unit),
                'network_critical': self.network_critical
            }
            base_dict.update(self.io_limits_serializable(unit))
            return base_dict

    def __init__(self, host):
        """
        :type host: core.hosts.Host
        """
        self.host = host
        self.cpu_model = host.model
        self.cpu_cores = host.ncpu
        self.memory = SaaSByteSize.from_int_bytes(host.get_avail_memory())
        self.memory_left = SaaSByteSize.from_int_bytes(host.get_avail_memory())
        self.cpu_left = int(host.power - get_system_used_power(host))
        self.ssd = SaaSByteSize('{} Gb'.format(host.ssd))
        self.hdd = SaaSByteSize('{} Gb'.format(host.disk))
        self.ssd_left = SaaSByteSize(self.ssd)
        self.hdd_left = SaaSByteSize(self.hdd)
        self.no_indexing = None
        self.groups_count = 0
        self.groups = set()
        self.assigned = []  # type: List[SaaSHostInfo.SaaSInstanceAssignedResources]
        groups = CURDB.groups.get_host_groups(self.host)
        for group in groups:
            instances = group.get_host_instances(self.host)
            for instance in instances:
                self.append_instance(instance)

    def __repr__(self):
        return '{switch}:{host};gc:{groups_count:02d};cpu:{cpu:04d};mem:{mem};ssd:{ssd}:noindex:{noindex}'.format(
            switch=self.switch, host=self.host.name, groups_count=self.groups_count,
            cpu=int(self.cpu_left), mem=self.memory_left.__repr__(suffix='Gb'), ssd=self.ssd_left.__repr__(suffix='Gb'), noindex=self.no_indexing
        )

    def __hash__(self):
        return hash(self.host.name)

    @property
    def switch(self):
        return self.host.rack or self.host.switch

    @property
    def net(self):
        return self.host.net

    @property
    def rack(self):
        return self.host.rack

    @property
    def name(self):
        return self.host.name

    @property
    def cpu_cores_left(self):
        return self.cpu_left/40.0

    @property
    def storage_dict(self):
        return self.host.storages

    @property
    def network_critical_groups(self):
        return set([g for g in self.groups if get_network_critical(g)])

    def has_network_critical_groups(self, ignore_list=None):
        if not ignore_list:
            ignore_list = set()
        else:
            ignore_list = set(ignore_list)
        return len(self.network_critical_groups.difference(ignore_list)) > 0

    def get_storage(self):
        """
        :rtype: core.hosts.StorageInfo
        """
        return self.host.get_storages()

    def append_instance(self, instance):
        """
        Append instance to host
        :type instance: core.instances.Instance
        """
        group = CURDB.groups.get_group(instance.type)

        assigned_resources = SaaSHostInfo.SaaSInstanceAssignedResources(instance, group)
        self.memory_left -= assigned_resources.memory
        self.cpu_left -= assigned_resources.cpu
        self.ssd_left -= assigned_resources.ssd
        self.hdd_left -= assigned_resources.hdd
        if self.host.dc.upper() not in self.IGNORE_NOINDEX_LOCATIONS:
            if assigned_resources.noindex is True:
                self.no_indexing = True
            elif assigned_resources.noindex is False and self.no_indexing is None:
                self.no_indexing = False

        if assigned_resources.is_saas:
            self.groups_count += 1

        self.assigned.append(assigned_resources)
        self.groups.add(group)

    def to_dict(self, measurement_unit=None):
        """
        :param measurement_unit: Memory and SSD would be measured in this unit (Gb for gigabytes)
        :type measurement_unit: AnyStr
        :rtype: dict
        """
        if measurement_unit:
            mem = self.memory_left.__repr__(suffix=measurement_unit)
            ssd = self.ssd_left.__repr__(suffix=measurement_unit)
            hdd = self.hdd_left.__repr__(suffix=measurement_unit)
        else:
            mem = self.memory_left.__repr__()
            ssd = self.ssd_left.__repr__()
            hdd = self.hdd_left.__repr__()
        return {
            'hostname': self.host.name,
            'avail_memory': mem,
            'avail_cpu': self.cpu_left,
            'avail_ssd': ssd,
            'avail_hdd': hdd,
            'no_indexing': self.no_indexing,
            'groups_count': self.groups_count,
            'switch': self.host.switch
        }

    def to_dict_full(self, measurement_unit=None):
        """
        :param measurement_unit: Memory and SSD would be measured in this unit (Gb for gigabytes)
        :type measurement_unit: [str, Unicode]
        :rtype: dict
        """
        base_dict = self.to_dict(measurement_unit=measurement_unit)
        if measurement_unit:
            total_mem_str = self.memory.__repr__(suffix=measurement_unit)
            total_ssd_str = self.ssd.__repr__(suffix=measurement_unit)
            total_hdd_str = self.hdd.__repr__(suffix=measurement_unit)
        else:
            total_mem_str = str(self.memory.total_mem)
            total_ssd_str = str(self.ssd)
            total_hdd_str = str(self.hdd)
        expansions = {
            'cpu_model': self.host.model,
            'cpu_cores': self.host.ncpu,
            'total_cpu': self.host.power,
            'total_mem': total_mem_str,
            'total_ssd': total_ssd_str,
            'total_hdd': total_hdd_str,
            'net': self.net,
            'instances': [i.to_dict() for i in sorted(self.assigned, key=operator.attrgetter('is_saas', 'memory', 'cpu', 'ssd'))]
        }
        base_dict.update(expansions)
        return base_dict

    def dump_for_yt(self):
        """
        :rtype: dict
        """
        return {
            'hostname': self.host.name,
            'dc': self.host.dc,
            'switch': self.host.switch,
            'net': self.net or 0,
            'cpu_model': self.host.model,
            'cpu_cores': self.host.ncpu,
            'cpu_power_total': int(self.host.power),
            'cpu_power_available': self.cpu_left,
            'ram_total': self.host.get_avail_memory(),
            'ram_available': self.memory_left.value,
            'ssd_total': self.ssd.value,
            'ssd_available': self.ssd_left.value,
            'hdd_total': self.hdd.value,
            'hdd_available': self.hdd_left.value,
            'noindexing': self.no_indexing,
            'instances': [i.to_dict(unit='B') for i in self.assigned]
        }

