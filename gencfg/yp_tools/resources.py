import collections
import itertools
import math
import logging
import core.db as db


class Resources(object):
    def __init__(self, power, mem, ssd=0, hdd=0):
        self.power, self.mem, self.ssd, self.hdd = int(power), int(mem), int(ssd), int(hdd)

    def __repr__(self):
        return 'Resources(power={}, mem={}, ssd={}, hdd={})'.format(
                          self.power, self.mem / 1024. ** 3, self.ssd, self.hdd)

    def __iadd__(self, other):
        self.power += other.power
        self.mem += other.mem
        self.ssd += other.ssd
        self.hdd += other.hdd
        return self


Flags = collections.namedtuple('Flags', ['avx2', 'gbit_1'])
Host = collections.namedtuple('Host', ['name', 'power', 'mem', 'ssd', 'hdd', 'rack', 'flags', 'groups'])

trace_hosts = {'vla2-6454.search.yandex.net'}


def gencfg_hosts(master, skip=None, backgrounds=None):
    skip = {group.name for group in skip}
    master = db.CURDB.groups.get_group(master)
    hosts = {host.name: Resources(host.power,
                                  host.get_avail_memory(),
                                  host.ssd, host.disk) for host in master.get_hosts()}
    raw_hosts = {host.name: host for host in master.get_hosts()}
    subtr = Resources(0, 0, 0, 0)
    total = Resources(0, 0, 0, 0)
    for host in hosts.itervalues():
        total.power += host.power
        total.mem += host.mem
        total.ssd += host.ssd
        total.hdd += host.hdd

    groups = collections.defaultdict(collections.Counter)
    for slave in itertools.chain(master.slaves, backgrounds or {}):
        if slave.card.name in skip:
            _log.debug('skip %s', slave.card.name)
            continue

        instance_power = 0
        instances = slave.get_kinda_busy_instances()
        if instances:
            instance_power = instances[0].power
            power = len(instances) * instance_power

        for instance in slave.get_kinda_busy_instances():
            hostname = instance.host.name
            host = hosts.get(hostname)
            if not host:
                continue  # host came from background group

            if slave.card.reqs.instances.get('cpu_policy') != 'idle':
                host.power -= instance.power
                subtr.power += instance.power

            host.hdd -= slave.card.reqs.instances.disk.gigabytes()
            host.ssd -= slave.card.reqs.instances.ssd.gigabytes()
            host.mem -= slave.card.reqs.instances.memory_guarantee.value
            subtr.hdd += slave.card.reqs.instances.disk.gigabytes()
            subtr.ssd += slave.card.reqs.instances.ssd.gigabytes()
            subtr.mem += slave.card.reqs.instances.memory_guarantee.value

            groups[hostname][slave.card.name] += 1

            if hostname in trace_hosts:
                logging.debug('%s %s %s %s', hostname, slave.card.name, int(host.mem), host.power)

    _log.info('total %s', total)
    _log.info('subtr %s', subtr)
    for name, host in hosts.iteritems():
        yield Host(name, int(host.power),
                   int(host.mem),
                   int(host.ssd), int(host.hdd), raw_hosts[name].rack,
                   flags=Flags(avx2=raw_hosts[name].model in ('Gold6230', 'E5-2660v4'),
                               gbit_1=raw_hosts[name].net <= 1024),
                   groups=groups[name])


_log = logging.getLogger(__name__)
