from collections import namedtuple
import itertools
import json
import logging
import math
import sys

import resources
from resources import Resources


def max_at_node(pods):
    return [{'max_pods': pods, 'key': 'node'}]


def max_at_rack(pods):
    return [{'max_pods': pods, 'key': 'rack'}]


def max_at(node, rack):
    return [{'max_pods': node, 'key': 'node'},
            {'max_pods': rack, 'key': 'rack'}]


class Group(namedtuple('Group', ['name', 'tier', 'replicas', 'shards', 'resources', 'antiaffinity', 'node_filter'])):
    @property
    def total(self):
        k = self.shards * self.replicas
        return Resources(k * self.resources.power,
                         k * self.resources.mem,
                         k * self.resources.ssd,
                         k * self.resources.hdd)

    def __hash__(self):
        return hash(self.name)

Group.__new__.__defaults__ = (max_at_node(1), None)  # antiaffinity, node_filter


def gb(value):
    return int(value) * 1024 ** 3 # have to account memory in bytes


def vcpu(value):
    return int(value * 1000)  # To avoid YtResponseError: "Invalid vcpu_gurantee value: expected >= 100, got 60"


def pod_set(pod_set_id, antiaffinity, node_filter):
    # should use rack-based antiaffinity, but not sure if it possible in simulation
    spec = {}
    if antiaffinity:
        spec['antiaffinity_constraints'] = antiaffinity
    if node_filter:
        spec['node_filter'] = node_filter
    return {'meta': {'type': 'pod_set', 'id': pod_set_id},
            'spec': spec}


def pod(pod_set_id, resources, replica):
    disk_volume_requests = []
    if resources.ssd:
        disk_volume_requests.append({'storage_class': 'ssd', 'id': 'ssd',
                                     'quota_policy': {'capacity': resources.ssd}})
    if resources.hdd:
        disk_volume_requests.append({'storage_class': 'hdd', 'id': 'hdd',
                                     'quota_policy': {'capacity': resources.hdd}})

    return {'labels': {'replica': str(replica)},
            'meta': {'type': 'pod', 'pod_set_id': pod_set_id},
            'spec': {'ip6_address_requests': [],
                     'resource_requests': {'memory_limit': resources.mem,
                                           'memory_guarantee': resources.mem,
                                           'vcpu_limit': vcpu(resources.power),
                                           'vcpu_guarantee': vcpu(resources.power)},
                     'disk_volume_requests': disk_volume_requests}}

def dump_group(group):
    pod_set_id = '{}'.format(group.name)
    yield pod_set(pod_set_id, group.antiaffinity, group.node_filter)
    for shard in range(group.shards):
        # single shard podsets cannot be used by gencfg
        # pod_set_id = '{}-shard-{}'.format(group.name, shard)
        # yield pod_set(pod_set_id, group.antiaffinity, group.node_filter)
        for replica in range(group.replicas):
            yield pod(pod_set_id, group.resources, replica)


def node(node_id, rack, labels=None):
    labels = labels or {}
    labels['topology'] = {'node': node_id, 'dc': 'SAS', 'rack': rack}
    return {'meta': {'type': 'node', 'id': node_id},
            'labels': labels}


def resource(node_id, kind, capacity):
    return {'meta': {'kind': kind, 'node_id': node_id, 'type': 'resource'},
            'spec': {kind: {'total_capacity': capacity}}}


def disk(node_id, storage_class, capacity):
    return {'meta': {'kind': 'disk', 'node_id': node_id, 'type': 'resource'},
            'spec': {'disk': {'supported_policies': ['quota', 'exclusive'],
                              'total_volume_slots': 100,
                              'storage_class': storage_class,
                              'total_capacity': capacity}}}


def dump_nodes(master, skip, backgrounds):
    total = Resources(0, 0)
    for host in resources.gencfg_hosts(master, skip, backgrounds):
        yield node(host.name, host.rack, labels={'cpu_flags': {'avx2': host.flags.avx2,
                                                               'gbit_1': host.flags.gbit_1}})  # not sure if scheduler supports non-cpu flags
        yield resource(host.name, 'cpu', max(vcpu(host.power), 1))
        yield resource(host.name, 'memory', host.mem)
        yield disk(host.name, 'hdd', host.hdd)
        yield disk(host.name, 'ssd', host.ssd)
        total += Resources(host.power, host.mem, host.ssd, host.hdd)

    logging.info('Free      %s', total)


def dump_groups(groups):
    total = Resources(0, 0)
    for group in groups:
        total += group.total
        for obj in dump_group(group):
            yield obj

    logging.info('Requested %s', total)


def gg(db, group):
    logging.info(group)
    shards = group.card.reqs.shards
    instances = group.card.reqs.instances
    tier = db.tiers.get_tier(shards.tier) if 'tier' in shards else None

    assert shards.equal_instances_power, 'Only equal instance power distribution allowed'
    replicas = getattr(shards, 'replicas', None) or shards.min_replicas
    assert shards.min_power == power_from_instance_power(group.card.legacy.funcs.instancePower), 'Power mismatch ' + group.card.name
    return Group(group.card.name, tier.name if tier else None,
                 replicas=replicas, shards=tier.get_shards_count() if tier else 1,
                 resources=Resources(power=shards.min_power, mem=instances.memory_guarantee.value,
                                     ssd=instances.ssd.gigabytes(), hdd=instances.disk.gigabytes()),
                 antiaffinity=antiaffinity_from_instance_count(group.card.legacy.funcs.instanceCount),
                 node_filter='[/labels/cpu_flags/avx2] = %true' if group.card.reqs.instances.net_guarantee.megabytes() > 100 else None)


def partial_recluster(relocate, wipe, master, backgrounds):
    # remove `relocate` and `wipe` from hosts
    nodes = list(dump_nodes(skip=relocate.union(wipe), master=master, backgrounds=backgrounds))
    # restore `relocate` back
    groups = list(dump_groups(relocate))
    return nodes + groups


def sas_baseline():
    import core.db
    db = core.db.CURDB
    todo = [
        'SAS_IMGS_THUMB_NEW', 'SAS_IMGS_LARGE_THUMB',
    ]
    groups = [
        'SAS_WEB_REMOTE_STORAGE_BASE_SLOTS', 'SAS_WEB_TIER1_EMBEDDING', 'SAS_WEB_TIER1_JUPITER_BASE', 'SAS_WEB_TIER1_INVERTED_INDEX',
        'SAS_WEB_GEMINI_BASE', 'SAS_WEB_TIER1_JUPITER_INT', 'SAS_WEB_INTL2',
        'SAS_WEB_CALLISTO_CAM_BASE', 'SAS_WEB_CALLISTO_CAM_INT', 'SAS_WEB_CALLISTO_CAM_INTL2',
        'SAS_IMGS_RIM_3K',
        'SAS_IMGS_T1_BASE', 'SAS_IMGS_T1_INT', 'SAS_IMGS_BASE', 'SAS_IMGS_INT', 'SAS_IMGS_CBIR_INT', 'SAS_IMGS_T1_CBIR_INT', 'SAS_IMGS_RECOMMEND_INT',
        'SAS_VIDEO_PLATINUM_BASE', 'SAS_VIDEO_TIER0_BASE', 'SAS_VIDEO_PLATINUM_INT', 'SAS_VIDEO_TIER0_INT'
    ]
    ggs = {gg(db, db.groups.get_group(group)) for group in groups}
    for g in sorted(ggs):
        logging.info('%s %s; %s x %s', g.name, g.resources, g.replicas, g.shards)
    return partial_recluster(
            master='SAS_WEB_BASE',
            relocate=ggs,
            wipe={},
            backgrounds=[db.groups.get_group('SAS_RTC_SLA_TENTACLES_PROD')]
    )

def antiaffinity_from_instance_count(s):
    assert s.startswith('exactly'), 'Only exact instance count allowed'
    return max_at_node(int(s.split('exactly')[1]))


def power_from_instance_power(s):
    assert s.startswith('exactly'), 'Only exact instance power allowed'
    return int(s.split('exactly')[1])


def get_group_spec(name):
    import core.db
    db = core.db.CURDB
    return gg(db, db.groups.get_group(name))


def man_baseline():
    import core.db
    db = core.db.CURDB
    todo = [
        'MAN_IMGS_THUMB_NEW', 'MAN_IMGS_LARGE_THUMB',
        'MAN_IMGS_RIM_3K',
    ]
    groups = ['MAN_WEB_REMOTE_STORAGE_BASE', 'MAN_WEB_TIER1_EMBEDDING', 'MAN_WEB_TIER1_JUPITER_BASE',
              'MAN_WEB_TIER1_INVERTED_INDEX', 'MAN_WEB_GEMINI_BASE', 'MAN_WEB_TIER1_JUPITER_INT', 'MAN_WEB_INTL2',
              'MAN_WEB_PLATINUM_JUPITER_BASE', 'MAN_WEB_PLATINUM_JUPITER_INT',
              'MAN_WEB_CALLISTO_CAM_BASE', 'MAN_WEB_CALLISTO_CAM_INT', 'MAN_WEB_CALLISTO_CAM_INTL2',
              'MAN_IMGS_T1_BASE', 'MAN_IMGS_T1_INT', 'MAN_IMGS_BASE', 'MAN_IMGS_INT', 'MAN_IMGS_CBIR_INT', 'MAN_IMGS_T1_CBIR_INT', 'MAN_IMGS_RECOMMEND_INT',
              'MAN_VIDEO_PLATINUM_BASE', 'MAN_VIDEO_TIER0_BASE', 'MAN_VIDEO_PLATINUM_INT', 'MAN_VIDEO_TIER0_INT']
    ggs = {gg(db, db.groups.get_group(group)) for group in groups}
    for g in sorted(ggs):
        logging.info('%s %s', g.name, g.resources)
    return partial_recluster(
            master='MAN_WEB_BASE',
            relocate=ggs,
            wipe={},
            backgrounds=[db.groups.get_group('MAN_RTC_SLA_TENTACLES_PROD')]
    )


def main(location):
    if location == 'man':
        print json.dumps(man_baseline(), indent=4)
    elif location == 'sas':
        print json.dumps(sas_baseline(), indent=4)
    else:
        raise RuntimeError('Unknown location {}'.format(location))


if __name__ == '__main__':
    # logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] %(message)s')
    logging.basicConfig(level=logging.DEBUG, format='%(message)s')

    main(sys.argv[1])
