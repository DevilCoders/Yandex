import collections
import random

import core.db
from core.instances import Instance


class Ho(object):
    def __init__(self, shards=None, slots=0):
        self.shards = set(shards) if shards else set()
        self.slots = int(slots)
        self.allocated = []

    def accepts(self, shard, colocate):
        if colocate:
            return shard in self.shards and self.accepts(shard, False)
        return len(self.allocated) < self.slots and shard not in self.allocated

    def allocate(self, shard):
       self.allocated.append(shard)


def test_Ho():
    h = Ho('h', ['s1', 's2', 's3'], 2)
    assert h.accepts('s1', True)
    assert h.accepts('s1', False)
    assert not h.accepts('Z', True)
    assert h.accepts('Z', False)

    h.allocate('s1')
    assert h.accepts('s1', True)
    assert h.accepts('s1', False)
    assert not h.accepts('Z', True)
    assert h.accepts('Z', False)

    h.allocate('Z')
    assert not h.accepts('s1', True)
    assert not h.accepts('s1', False)
    assert not h.accepts('Z', True)
    assert not h.accepts('Z', False)


def allocate(hosts, shards):
    # first pass: colocate shards
    colocated = []
    not_colocated = []
    for shard in shards:
        for host in hosts:
            if host.accepts(shard, colocate=True):
                host.allocate(shard)
                colocated.append(shard)
                break
        else:
            not_colocated.append(shard)
    print 'colocated', len(colocated), 'instances'

    # second pass: allocate shards randomly
    random.shuffle(not_colocated)
    for shard in not_colocated:
        for host in hosts:
            if host.accepts(shard, colocate=False):
                host.allocate(shard)
                break
        else:
            raise Exception('Unable to allocate')
    print 'allocated', len(not_colocated), 'instances'

    return hosts


def test_allocate():
    hosts = allocate([Ho(['s1', 's2'], 1),
                      Ho(['s1', 's2'], 1),
                      Ho([], 1)],
                     ['s1', 's2', 's3'])


def colocate_intlookups(prod, pip):
    hosts = collections.defaultdict(Ho)
    for shard in range(prod.get_shards_count()):
        for instance in prod.get_base_instances_for_shard(shard):
            hosts[instance.host.name].shards.add(shard)

    for instance in pip.get_instances():
        hosts[instance.host.name].slots += 1
    shards = range(prod.get_shards_count()) + range(prod.get_shards_count())
    allocate(hosts.values(), shards)
    return hosts


def do():
    db = core.db.CURDB
    prod = db.intlookups.get_intlookup('VLA_WEB_TIER1_INVERTED_INDEX')
    pip = db.intlookups.get_intlookup('VLA_WEB_TIER1_INVERTED_INDEX_PIP')

    hs = colocate_intlookups(prod, pip)

    base_port = db.groups.get_group(pip.base_type).get_default_port()
    instances = collections.defaultdict(list)
    for name, ho in hs.iteritems():
        for i, shard in enumerate(ho.allocated):
            instances[shard].append(Instance(db.hosts.get_host_by_name(name), 1.0, base_port + i * 8, str(pip.base_type), 0))

    shard = 0
    for multishard in pip.get_multishards():
        for i, brigade in enumerate(multishard.brigades):
            brigade.basesearchers = [[instances[shard][i]]]
        shard += 1

    pip.write_intlookup_to_file_json('pip.json')


if __name__ == '__main__':
    test_Ho()
    test_allocate()
