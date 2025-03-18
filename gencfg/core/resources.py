import ujson
import copy

from core.db import CURDB
from core.card.types import ByteSize

MB = 1024 * 1024
GB = MB * 1024


def hr(v):
    if v > 100 * MB:
        return '{:.2f} Gb'.format(float(v) / GB)
    else:
        return '{:.2f} Mb'.format(float(v) / MB)


class TResources(object):
    """Class with resources and arithmetic"""

    __slots__ = [
        'cpu',  # (in PU)
        'memory',  #  (in bytes)
        'hdd',  # (in bytes)
        'ssd',  # (in bytes)
        'net',  # (in bytes)
    ]

    def __init__(self, cpu=0, memory=0, hdd=0, ssd=0, net=0):
        """Simple constructor"""

        self.cpu = cpu
        self.memory = memory
        self.hdd = hdd
        self.ssd = ssd
        self.net = net


    @classmethod
    def from_host(cls, host):
        """
            Constructor, initializing object from specified host. All resources are equal to total host resources

            :type host: core.hosts.Host

            :param host: host, which resources we used to create object
            :return (core.resources.TAllocedResources): internal gencfg resources object
        """
        GB = 1024 * 1024 * 1024

        cpu = host.power
        memory = host.memory * GB
        ssd = max(host.ssd, host.ssd_size) * GB
        hdd = max(host.disk, host.hdd_size) * GB
        net = host.net * 1024 * 1024 / 8

        return cls(cpu=cpu, memory=memory, ssd=ssd, hdd=hdd, net=net)

    @classmethod
    def from_instance(cls, instance, db=CURDB):
        """
            Constructor, initializing object from Instance type. Used to create TAllocedResources objects in
            old dbs

            :type instance: core.instances.Instance

            :param instance: internal gencfg instance structure
            :return (core.resources.TAllocatedResources): internal gencfg resources object
        """
        group = db.groups.get_group(instance.type)

        cpu = instance.power
        memory = group.card.reqs.instances.memory_guarantee.value
        ssd = group.card.reqs.instances.ssd.value
        hdd = group.card.reqs.instances.disk.value
        net = group.card.reqs.instances.net_guarantee.value

        return cls(cpu=cpu, memory=memory, ssd=ssd, hdd=hdd, net=net)

    @classmethod
    def used_resources(cls, host, db=CURDB, ignore_fullhost=False):
        used_resources = cls()
        for instance in db.groups.get_host_instances(host):
            if ignore_fullhost and db.groups.get_group(instance.type).card.legacy.funcs.instancePower == 'fullhost':
                continue
            used_resources += cls.from_instance(instance, db=db)
        return used_resources

    @classmethod
    def free_resources(cls, host, db=CURDB):
        """Construct free host resources"""
        return cls.from_host(host) - cls.used_resources(host)

    @classmethod
    def free_resources_skip_master(cls, host, db=CURDB):
        result = cls.free_resources(host, db=db)
        master_group = db.groups.get_host_master_group(host)
        result += cls.used_group_resources(host, master_group, db=db)
        return result

    @classmethod
    def used_group_resources(cls, host, group, db=CURDB):
        result = cls()
        if group.hasHost(host):
            for instance in group.get_host_instances(host):
                result += cls.from_instance(instance)
        return result

    @classmethod
    def from_line(cls, line):
        """Construct from line like <power=0,memory=13 Gb,...>"""
        data = {x.partition('=')[0]:x.partition('=')[2] for x in line.strip().split(',')}

        unknown_keys = set(data.keys()) - set(['cpu', 'memory', 'ssd', 'hdd', 'net'])
        if unknown_keys:
            raise Exception('Could not parse line <{}> as resources: unknown keys <{}>'.format(line, ','.join(unknown_keys)))

        cpu = int(data.get('cpu', 0))
        memory = ByteSize(data.get('memory', '0 Gb')).value
        ssd = ByteSize(data.get('ssd', '0 Gb')).value
        hdd = ByteSize(data.get('hdd', '0 Gb')).value
        net = ByteSize(data.get('net', '0 Gb')).value

        return cls(cpu=cpu, memory=memory, ssd=ssd, hdd=hdd, net=net)

    def can_subtract(self, other):
        """Check if our resources bigger than other"""
        status = (self.cpu >= other.cpu) and (self.memory >= other.memory) and (self.ssd >= other.ssd) and \
                 (self.hdd >= other.hdd) and (self.net >= other.net)
        return status

    # arithmetic
    def __iadd__(self, rhs):
        self.cpu += rhs.cpu
        self.memory += rhs.memory
        self.ssd += rhs.ssd
        self.hdd += rhs.hdd
        self.net += rhs.net

        return self

    def __add__(self, rhs):
        result = copy.copy(self)
        result += rhs
        return result

    def __isub__(self, rhs):
        self.cpu -= rhs.cpu
        self.memory -= rhs.memory
        self.ssd -= rhs.ssd
        self.hdd -= rhs.hdd
        self.net -= rhs.net

        return self

    def __sub__(self, rsh):
        result = copy.copy(self)
        result -= rsh
        return result

    # str operators
    def __str__(self):
        return 'cpu={cpu:.2f} Pu, memory={memory}, ssd={ssd}, hdd={hdd}, net={net}'.format(
                    cpu=self.cpu, memory=hr(self.memory), ssd=hr(self.ssd), hdd=hr(self.hdd), net=hr(self.net),
               )
