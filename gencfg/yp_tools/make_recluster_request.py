# coding=utf-8

import core.db

import collections
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


class Group(collections.namedtuple('Group', ['name', 'tier', 'replicas', 'shards', 'resources', 'antiaffinity', 'node_filter'])):
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


def dump_nodes(masters, skip, backgrounds, reserve):
    total = Resources(0, 0)

    # Filter nodes.
    banned_hosts = set()
    banned = 0
    # with open('db/groups/VLA_YT_RTC/VLA_WEB_TIER0_BUILD.hosts') as f:
    #   banned_hosts |= set(l.strip() for l in f)
    #banned_hosts |= {'vla1-6203.search.yandex.net'}
    #banned_hosts = list(banned_hosts)[0:500]
    banned_hosts = frozenset(banned_hosts)
    banned_power = 0
    overcommitted = []
    overcommitted_ssd = []
    overcommitted_hdd = []
    racks = collections.defaultdict(int)

    for master in masters:
        for host in resources.gencfg_hosts(master, skip, backgrounds):
            racks[host.rack] += 1
            #if racks[host.rack] > 6:
            #   continue

            if host.name in banned_hosts:
                banned += 1
                banned_power += host.power
                continue
            if host.mem < 0:
                print host.name
                raise Exception('wtf')

            if host.power < 0:
                overcommitted.append(host.name)
            if host.ssd < 0:
                overcommitted_ssd.append(host.name)
            if host.hdd < 0:
                overcommitted_hdd.append(host.name)

            # Try to reserve cpu for remote storage
            # if host.ssd > 256:
            #    free_cpu = host.power - reserve.power
            #    if free_cpu <= 0:
            #        continue
            # else:
            #    free_cpu = host.power
            # yield resource(host.name, 'cpu', max(vcpu(free_cpu), 1))

            yield resource(host.name, 'cpu', max(vcpu(host.power - reserve.power), 1))
            yield node(host.name, host.rack, labels={'cpu_flags': {'avx2': host.flags.avx2,
                                                                   'gbit_1': host.flags.gbit_1}})
            # Reserve memory for deployers and remote storage.
            yield resource(host.name, 'memory', max(host.mem - reserve.mem, 0))
            yield disk(host.name, 'hdd', max(host.hdd - reserve.hdd, 1))
            yield disk(host.name, 'ssd', max(host.ssd - reserve.ssd, 1))

            total += Resources(host.power, host.mem, host.ssd, host.hdd)

    #for extra in xrange(60):
    #   name = 'extra' + str(extra) + '-' + host.name
    #   yield node(name, host.rack, labels={'cpu_flags': {'avx2': host.flags.avx2,
    #                                                     'gbit_1': host.flags.gbit_1}})  # not sure if scheduler supports non-cpu flags
    #   # yield resource(name, 'cpu', max(vcpu(host.power), 1))
    #   yield resource(name, 'cpu', vcpu(2539.0))
    #   yield resource(name, 'memory', gb(240))
    #   yield disk(name, 'hdd', host.hdd)
    #   yield disk(name, 'ssd', 0)
    #   # total += Resources(host.power, host.mem, ssd, host.hdd)
    #   total += Resources(2539.0, gb(240), 0, host.hdd)

    logging.info('Overcommitted by cpu %s hosts', len(overcommitted))
    logging.info('Overcommitted by ssd %s hosts', len(overcommitted_ssd))
    logging.info('Overcommitted by hdd %s hosts', len(overcommitted_hdd))
    logging.info('Banned %s hosts of %s power', banned, banned_power)
    logging.info('Free      %s', total)
    with open('./overcommitted_ssd', 'w') as f:
        print >> f, '\n'.join(overcommitted_ssd)


def dump_groups(groups):
    total = Resources(0, 0)
    for group in groups:
        total += group.total
        for obj in dump_group(group):
            yield obj

    logging.info('Requested %s', total)


def group_requirements(db, group):
    logging.info(group)
    shards = group.card.reqs.shards
    instances = group.card.reqs.instances
    tier = db.tiers.get_tier(shards.tier) if 'tier' in shards else None

    # assert shards.equal_instances_power, 'Only equal instance power distribution allowed'
    replicas = getattr(shards, 'replicas', None) or shards.min_replicas
    # assert shards.min_power == power_from_instance_power(group.card.legacy.funcs.instancePower), 'Power mismatch ' + group.card.name
    node_filter = '[/labels/cpu_flags/avx2] = %true' if 'EMBEDDING' in group.card.name else None
    return Group(group.card.name, tier.name if tier else None,
                 replicas=replicas, shards=tier.get_shards_count() if tier else 1,
                 resources=Resources(power=shards.min_power, mem=instances.memory_guarantee.value,
                                     ssd=instances.ssd.gigabytes(), hdd=instances.disk.gigabytes()),
                 antiaffinity=antiaffinity_from_card(group.card),
                 node_filter=node_filter)
                 # node_filter='[/labels/cpu_flags/gbit_1] = %false' if group.card.reqs.instances.net_guarantee.megabytes() > 100 else None)

gg = group_requirements  # legacy; rm


def partial_recluster(relocate, wipe, master, backgrounds, extra_host_donors=None, reserve=None):
    # default reserve
    if reserve is None:
       reserve = Resources(power=0, mem=gb(0), ssd=0, hdd=0)

    masters = [master]
    if extra_host_donors:
        masters.extend(extra_host_donors)

    db = core.db.CURDB
    # remove `relocate` and `wipe` from hosts
    erase = {group_requirements(db, group) for group in relocate + wipe}
    for group in erase:
        logging.info('Recluster %s', group)
    nodes = list(dump_nodes(masters, erase, backgrounds=backgrounds, reserve=reserve))
    # restore `relocate` back
    allocate = {group_requirements(db, group) for group in relocate}
    groups = list(dump_groups(allocate))
    return nodes + groups


def sas_baseline():
    db = core.db.CURDB
    groups = [
        'SAS_VIDEO_REFRESH_DISTRIBUTOR',
    ]
    parsed_groups = {group_requirements(db, db.groups.get_group(group)) for group in groups}
    for g in sorted(parsed_groups):
        logging.info('%s %s; %s x %s', g.name, g.resources, g.replicas, g.shards)
    return partial_recluster(
        master='SAS_WEB_BASE',
        relocate=parsed_groups,
        wipe={},
        backgrounds=[db.groups.get_group('SAS_RTC_SLA_TENTACLES_PROD')]
    )


def antiaffinity_from_card(card):
    if card.reqs.hosts.get('max_per_switch'):
        return antiaffinity_from_instance_count(card.legacy.funcs.instanceCount) + \
               max_at_rack(int(card.reqs.hosts['max_per_switch']))
    return antiaffinity_from_instance_count(card.legacy.funcs.instanceCount)


def antiaffinity_from_instance_count(s):
    assert s.startswith('exactly'), 'Only exact instance count allowed'
    # return max_at_node(1)
    return max_at_node(int(s.split('exactly')[1]))
    # return max_at_node(max(int(s.split('exactly')[1]), 3))
    # return max_at_node(2)


def power_from_instance_power(s):
    assert s.startswith('exactly'), 'Only exact instance power allowed'
    return int(s.split('exactly')[1])


def get_group_spec(name):
    db = core.db.CURDB
    return gg(db, db.groups.get_group(name))


def man_check_cpu():
    db = core.db.CURDB
    # cohabitation: cpu, mem, hdd, a few ssd
    # base group: full ssd
    cohab = db.groups.get_group('MAN_YP_COHABITATION_TIER1_BASE')
    platinum = db.groups.get_group('MAN_WEB_PLATINUM_JUPITER_BASE')
    nodes = list(dump_nodes(master='MAN_WEB_BASE',  # why not master=db.groups.get_group('MAN_WEB_BASE')??
                            backgrounds=[db.groups.get_group('MAN_RTC_SLA_TENTACLES_PROD')],
                            skip=set()))
    logging.info('%s nodes', len(nodes) / 5)


def man_recluster_platinum():
    db = core.db.CURDB

    wipe = [
        'MAN_YP_COHABITATION_PLATINUM_BASE',
        'MAN_YP_COHABITATION_TIER1_BASE'
    ]

    # platinum:
    # 400 pu instead of 280
    # setup other resources, take from MAN_YP_COHABITATION_PLATINUM_BASE
    platinum = db.groups.get_group('MAN_WEB_PLATINUM_JUPITER_BASE')
    platinum.card.reqs.shards.min_power = 400
    platinum.card.reqs.instances.memory_guarantee.value = (16 + 11) * 1024. ** 3  # + 11 Gb MULTI
    platinum.card.reqs.instances.ssd.value += (15 + 15) * 1024. ** 3
    platinum.card.reqs.instances.disk.value += 50 * 1024. ** 3

    # tier1 reqs
    tier1 = db.groups.get_group('MAN_WEB_TIER1_JUPITER_BASE')
    tier1.card.reqs.shards.min_power = 720
    tier1.card.reqs.instances.memory_guarantee.value = (120 + 11) * 1024. ** 3  # + 11 Gb MULTI
    tier1.card.reqs.instances.ssd.value = (820 + 15) * 1024. ** 3
    tier1.card.reqs.instances.disk.value = 5 * 1024. ** 3
    groups = {group_requirements(db, db.groups.get_group(group)) for group in [
        'MAN_WEB_PLATINUM_JUPITER_BASE', 'MAN_WEB_PLATINUM_JUPITER_INT',
        'MAN_WEB_TIER1_JUPITER_BASE', 'MAN_WEB_TIER1_JUPITER_INT',
        'MAN_WEB_TIER1_EMBEDDING',
        'MAN_WEB_INTL2',
        #'MAN_WEB_CALLISTO_CAM_BASE', 'MAN_WEB_CALLISTO_CAM_INT', 'MAN_WEB_CALLISTO_CAM_INTL2',
        'MAN_VIDEO_PLATINUM_BASE', 'MAN_VIDEO_TIER0_BASE', 'MAN_VIDEO_PLATINUM_INT', 'MAN_VIDEO_TIER0_INT',
    ]}
    data = partial_recluster(
            master='MAN_WEB_BASE',
            relocate=groups,
            wipe=set(gg(db, db.groups.get_group(group)) for group in wipe),
            backgrounds=[db.groups.get_group('MAN_RTC_SLA_TENTACLES_PROD')]
    )
    with open('man_recluster_platinum.json', 'w') as f:
        json.dump(sorted(data), f, indent=4, sort_keys=True)


def man_full():
    db = core.db.CURDB
    todo = [
        'MAN_IMGS_THUMB_NEW', 'MAN_IMGS_LARGE_THUMB',
    ]
    recluster = ['MAN_WEB_REMOTE_STORAGE_BASE', 'MAN_WEB_TIER1_EMBEDDING', 'MAN_WEB_TIER1_JUPITER_BASE',
                 'MAN_WEB_TIER1_INVERTED_INDEX', 'MAN_WEB_GEMINI_BASE', 'MAN_WEB_TIER1_JUPITER_INT', 'MAN_WEB_INTL2',
                 'MAN_WEB_PLATINUM_JUPITER_BASE', 'MAN_WEB_PLATINUM_JUPITER_INT',
                 'MAN_WEB_CALLISTO_CAM_BASE', 'MAN_WEB_CALLISTO_CAM_INT', 'MAN_WEB_CALLISTO_CAM_INTL2',
                 'MAN_IMGS_T1_BASE', 'MAN_IMGS_T1_INT', 'MAN_IMGS_BASE', 'MAN_IMGS_INT', 'MAN_IMGS_CBIR_INT', 'MAN_IMGS_T1_CBIR_INT', 'MAN_IMGS_RECOMMEND_INT',
                 'MAN_VIDEO_PLATINUM_BASE', 'MAN_VIDEO_TIER0_BASE', 'MAN_VIDEO_PLATINUM_INT', 'MAN_VIDEO_TIER0_INT',
                 'MAN_IMGS_RIM_3K', 'MAN_DISK_LUCENE', 'MAN_MAIL_LUCENE']
    wipe = ['MAN_YP_COHABITATION_PLATINUM_BASE', 'MAN_YP_COHABITATION_TIER1_BASE']
    K = 0.91
    # fix platinum
    platinum = db.groups.get_group('MAN_WEB_PLATINUM_JUPITER_BASE')
    platinum.card.reqs.shards.min_power = 400 * K
    platinum.card.reqs.instances.memory_guarantee.value = 16 * 1024. ** 3
    platinum.card.reqs.instances.ssd.value += 15 * 1024. ** 3
    platinum.card.reqs.instances.disk.value += 50 * 1024. ** 3

    # fix tier1
    tier1 = db.groups.get_group('MAN_WEB_TIER1_JUPITER_BASE')
    tier1.card.reqs.shards.min_power = 720 * K
    tier1.card.reqs.instances.memory_guarantee.value = 120 * 1024. ** 3
    tier1.card.reqs.instances.ssd.value = 820 * 1024. ** 3
    tier1.card.reqs.instances.disk.value = 5 * 1024. ** 3

    # fix invindex
    invindex = db.groups.get_group('MAN_WEB_TIER1_INVERTED_INDEX')
    invindex.card.reqs.shards.min_power = 40
    invindex.card.reqs.instances.memory_guarantee.value = 3 * invindex.card.reqs.instances.memory_guarantee.value
    invindex.card.reqs.instances.ssd.value = 3 * invindex.card.reqs.instances.ssd.value
    invindex.card.reqs.instances.disk.value = 3 * invindex.card.reqs.instances.disk.value

    # new lucene
    mail = db.groups.get_group('MAN_MAIL_LUCENE')
    mail.card.legacy.funcs.instanceCount = "exactly10"
    mail.card.reqs.instances.memory_guarantee.value = 20 * 1024. ** 3
    mail.card.reqs.instances.ssd.value = 800 * 1024. ** 3
    mail.card.reqs.instances.disk.value = 120 * 1024. ** 3
    disk = db.groups.get_group('MAN_DISK_LUCENE')
    disk.card.legacy.funcs.instanceCount = "exactly10"
    disk.card.reqs.instances.memory_guarantee.value = 20 * 1024. ** 3
    disk.card.reqs.instances.ssd.value = 850 * 1024. ** 3
    disk.card.reqs.instances.disk.value = 120 * 1024. ** 3
    data = partial_recluster(
            master='MAN_WEB_BASE',
            relocate={group_requirements(db, db.groups.get_group(group)) for group in recluster},
            wipe=set(group_requirements(db, db.groups.get_group(group)) for group in wipe),
            backgrounds=[db.groups.get_group('MAN_RTC_SLA_TENTACLES_PROD')]
    )
    with open('full.json', 'w') as f:
        json.dump(data, f, indent=4)


def vla_baseline():
    db = core.db.CURDB
    groups = [
        'VLA_IMGS_INT',
        'VLA_IMGS_CBIR_INT',
        'VLA_IMGS_RECOMMEND_INT',
        'VLA_IMGS_T1_INT',
        'VLA_IMGS_T1_CBIR_INT',
    ]
    imgs = db.groups.get_group('VLA_IMGS_INT')
    imgs.card.legacy.funcs.instanceCount = 'exactly2'
    imgs.card.reqs.shards.min_power = 120
    ggs = {gg(db, db.groups.get_group(group)) for group in groups}

    return partial_recluster(master='VLA_YT_RTC', relocate=ggs, wipe=set(), backgrounds=[])


def vla_baseline_plus_video():
    db = core.db.CURDB
    groups = [
        'VLA_VIDEO_PLATINUM_INT',
        'VLA_VIDEO_TIER0_INT',

        'VLA_IMGS_INT',
        'VLA_IMGS_CBIR_INT',
        'VLA_IMGS_RECOMMEND_INT',
        'VLA_IMGS_T1_INT',
        'VLA_IMGS_T1_CBIR_INT',
    ]
    platinum = db.groups.get_group('VLA_VIDEO_PLATINUM_INT')
    platinum.card.legacy.funcs.instanceCount = 'exactly2'
    platinum.card.reqs.shards.min_power = 160

    tier0 = db.groups.get_group('VLA_VIDEO_TIER0_INT')
    tier0.card.legacy.funcs.instanceCount = 'exactly2'
    tier0.card.reqs.shards.min_power = 160

    imgs = db.groups.get_group('VLA_IMGS_INT')
    imgs.card.legacy.funcs.instanceCount = 'exactly2'
    imgs.card.reqs.shards.min_power = 120

    ggs = {gg(db, db.groups.get_group(group)) for group in groups}

    return partial_recluster(master='VLA_YT_RTC', relocate=ggs, wipe=set(), backgrounds=[])


def vla_4144():
    db = core.db.CURDB
    platinum = db.groups.get_group('VLA_VIDEO_PLATINUM_EMBEDDING')
    tier0 = db.groups.get_group('VLA_VIDEO_TIER0_EMBEDDING')
    imgs = db.groups.get_group('VLA_IMGS_TIER0_EMBEDDING')

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        platinum.card.name,
        tier0.card.name,
        imgs.card.name
    ]}
    return partial_recluster(master='GENCFG_4144', relocate=reqs, wipe=set(), backgrounds=[])


def vla_webtier0_pip():
    db = core.db.CURDB
    builder = db.groups.get_group('VLA_WEB_TIER0_BUILD')
    builder.card.reqs.instances.memory_guarantee.value = (70) * 1024. ** 3
    basesearch = db.groups.get_group('VLA_WEB_TIER0_BASE_PIP')
    embedding = db.groups.get_group('VLA_WEB_TIER0_EMBEDDING_PIP')
    invindex = db.groups.get_group('VLA_WEB_TIER0_INVERTED_INDEX_PIP')
    intl1 = db.groups.get_group('VLA_WEB_TIER0_INT_PIP')
    intl2 = db.groups.get_group('VLA_WEB_TIER0_INTL2_PIP')
    rs = db.groups.get_group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE_PIP')
    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        builder.card.name,
    ]}
    return partial_recluster(master='VLA_YT_RTC', relocate=reqs, wipe=set(), backgrounds=[])


def vla_webtier0_prod():
    db = core.db.CURDB
    basesearch = db.groups.get_group('VLA_WEB_TIER0_BASE')
    basesearch.card.reqs.shards.min_power = 480
    embedding = db.groups.get_group('VLA_WEB_TIER0_EMBEDDING')
    invindex = db.groups.get_group('VLA_WEB_TIER0_INVERTED_INDEX')
    intl1 = db.groups.get_group('VLA_WEB_TIER0_INT')
    intl2 = db.groups.get_group('VLA_WEB_TIER0_INTL2')
    rs = db.groups.get_group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE')
    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        basesearch.card.name,
        embedding.card.name,
        invindex.card.name,
        intl1.card.name,
        intl2.card.name,
        rs.card.name,
    ]}
    return partial_recluster(master='VLA_YT_RTC', relocate=reqs, wipe=set(), backgrounds=[])


def vla_imgs_pip():
    db = core.db.CURDB
    embedding = db.groups.get_group('VLA_IMGS_EMBEDDING_PIP')
    invindex = db.groups.get_group('VLA_IMGS_INVERTED_INDEX_PIP')
    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        embedding.card.name,
        invindex.card.name
    ]}
    return partial_recluster(master='VLA_YT_RTC', relocate=reqs, wipe=set(), backgrounds=[])


def sas_4375():
    db = core.db.CURDB
    platinum = db.groups.get_group('SAS_VIDEO_PLATINUM_EMBEDDING')
    tier0 = db.groups.get_group('SAS_VIDEO_TIER0_EMBEDDING')
    imgs = db.groups.get_group('SAS_IMGS_TIER0_EMBEDDING')

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        platinum.card.name,
        tier0.card.name,
        imgs.card.name
    ]}
    return partial_recluster(master='GENCFG_4375_SAS', relocate=reqs, wipe=set(), backgrounds=[])


def sas_1569():
    db = core.db.CURDB
    mail = db.groups.get_group('SAS_MAIL_LUCENE')
    mail.card.reqs.instances.ssd.value = 900 * 1024. ** 3
    mail.card.reqs.instances.disk.value = 305 * 1024. ** 3

    disk = db.groups.get_group('SAS_DISK_LUCENE')
    disk.card.reqs.instances.ssd.value = 950 * 1024. ** 3
    disk.card.reqs.instances.disk.value = 275 * 1024. ** 3

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        mail.card.name,
        disk.card.name,
    ]}
    return partial_recluster(master='SAS_WEB_BASE', relocate=reqs, wipe=set(), backgrounds=[])


def sas_new_runtime():
    db = core.db.CURDB
    tier0 = db.groups.get_group('SAS_VIDEO_TIER0_INT')
    tier0.card.reqs.shards.min_power = 160
    platinum = db.groups.get_group('SAS_VIDEO_PLATINUM_INT')
    platinum.card.reqs.shards.min_power = 160
    imgs = db.groups.get_group('SAS_IMGS_INT')
    imgs.card.reqs.shards.min_power = 120

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        'SAS_IMGS_INT',
        'SAS_IMGS_INVERTED_INDEX',
        'SAS_VIDEO_PLATINUM_INT',
        'SAS_VIDEO_PLATINUM_INVERTED_INDEX',
        'SAS_VIDEO_TIER0_INT',
        'SAS_VIDEO_TIER0_INVERTED_INDEX',
    ]}
    return partial_recluster(master='SAS_WEB_BASE', relocate=reqs, wipe=set(), backgrounds=[])


def vla_gencfg_4396_huge_ssd():
    # https://st.yandex-team.ru/GENCFG-4396#5f889b1fa6dfa025fe13c88f
    # Succesfully converged.
    # Remains unsatisfied: lucene cpu requirements.
    # Relocate & lower ssd requirements: remote storage (pip & prod), t1 builder.
    db = core.db.CURDB

    mail = db.groups.get_group('VLA_MAIL_LUCENE')
    mail.card.reqs.instances.ssd.value = 800 * 1024. ** 3
    mail.card.reqs.instances.disk.value = 155 * 1024. ** 3
    mail.card.reqs.instances.memory_guarantee.value = 20 * 1024. ** 3
    # mail.card.reqs.shards.min_power = 140

    disk = db.groups.get_group('VLA_DISK_LUCENE')
    disk.card.reqs.instances.ssd.value = 850 * 1024. ** 3
    disk.card.reqs.instances.disk.value = 125 * 1024. ** 3
    disk.card.reqs.instances.memory_guarantee.value = 20 * 1024. ** 3
    # disk.card.reqs.shards.min_power = 200

    rs = db.groups.get_group('VLA_WEB_TIER1_REMOTE_STORAGE_BASE')
    rs.card.reqs.instances.ssd.value = 780 * 1024. ** 3  # 1024 originally

    rspip = db.groups.get_group('VLA_WEB_TIER1_REMOTE_STORAGE_BASE_PIP')
    rspip.card.reqs.shards.min_power = 1

    builder = db.groups.get_group('VLA_WEB_TIER1_BUILD')
    builder.card.reqs.instances.ssd.value = 400 * 1024. ** 3  # 1600 originally

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        mail.card.name,
        disk.card.name,
        rs.card.name,
        rspip.card.name,
        builder.card.name,
    ]}
    return partial_recluster(master='VLA_YT_RTC', relocate=reqs,
                             wipe={
                                 # group_requirements(db, db.groups.get_group('VLA_YP_COHABITATION_TIER1_BASE')),
                             }, backgrounds=[])


def _vla_gencfg_4142_web():
    basesearch = group('VLA_WEB_TIER0_BASE')
    basesearch.card.reqs.shards.min_power = 480  # from cohabitation
    basesearch.card.reqs.instances.memory_guarantee.value = 125 * Gb  # from cohabitation
    embedding = group('VLA_WEB_TIER0_EMBEDDING')
    invindex = group('VLA_WEB_TIER0_INVERTED_INDEX')
    intl1 = group('VLA_WEB_TIER0_INT')
    # intl1.card.reqs.shards.min_power = 80  # 200 originally
    intl2 = group('VLA_WEB_TIER0_INTL2')
    rstier0 = group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE')

    callisto = group('VLA_WEB_CALLISTO_CAM_BASE')
    callisto.card.reqs.instances.memory_guarantee.value += 1 * Gb  # hamster

    return [
        basesearch,
        embedding,
        invindex,
        intl1,
        intl2,
        rstier0,
        callisto,
        group('VLA_WEB_CALLISTO_CAM_INT'),
        group('VLA_WEB_CALLISTO_CAM_INTL2'),
    ]


def group(group_name):
    class Wrapper(object):
        def __init__(self, gencfg_group):
            self.gencfg_group = gencfg_group

        def __getattr__(self, name):
            return getattr(self.gencfg_group, name)

        def __str__(self):
            return str(self.gencfg_group)

        def consume(self, other_group):
            self.ssd += other_group.ssd
            self.disk += other_group.disk
            self.power += other_group.power
            self.mem += other_group.mem
            other_group.ssd = 0
            other_group.disk = 0
            other_group.power = 0
            other_group.mem = 0

        @property
        def mem(self):
            return self.card.reqs.instances.memory_guarantee.value

        @mem.setter
        def mem(self, value):
            self.card.reqs.instances.memory_guarantee.value = value

        @property
        def power(self):
            return self.card.reqs.shards.min_power

        @power.setter
        def power(self, value):
            self.card.reqs.shards.min_power = value
            self.card.legacy.funcs.instancePower = 'exactly{}'.format(int(value))
            for instance in self.get_kinda_busy_instances():
                instance.power = value

        @property
        def disk(self):
            return self.card.reqs.instances.disk.value

        @disk.setter
        def disk(self, value):
            self.card.reqs.instances.disk.value = value

        @property
        def ssd(self):
            return self.card.reqs.instances.ssd.value

        @ssd.setter
        def ssd(self, value):
            self.card.reqs.instances.ssd.value = value

    grp = core.db.CURDB.groups.get_group(group_name)
    if grp.card.reqs.instances.get('cpu_policy') == 'idle':
        grp.card.reqs.shards.min_power = 1
    return Wrapper(grp)


Gb = 1024 ** 3



def _vla_gencfg_4142_pips():
    return [group('VLA_IMGS_EMBEDDING_PIP'), group('VLA_IMGS_INVERTED_INDEX_PIP'),
            group('VLA_WEB_TIER0_BASE_PIP'), group('VLA_WEB_TIER0_EMBEDDING_PIP'),
            group('VLA_WEB_TIER0_INVERTED_INDEX_PIP'),
            group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE_PIP')]


def _vla_gencfg_4142_video():
    emb_pt = group('VLA_VIDEO_PLATINUM_EMBEDDING')
    emb_pt.card.reqs.disk = 10 * Gb
    emb_pt.card.reqs.ssd = 50 * Gb
    emb_t0 = group('VLA_VIDEO_TIER0_EMBEDDING')
    emb_t0.card.reqs.disk = 10 * Gb
    emb_t0.card.reqs.ssd = 50 * Gb
    ii_pt = group('VLA_VIDEO_PLATINUM_INVERTED_INDEX')
    ii_t0 = group('VLA_VIDEO_TIER0_INVERTED_INDEX')
    ints_pt = group('VLA_VIDEO_PLATINUM_INT')
    ints_t0 = group('VLA_VIDEO_TIER0_INT')
    base_pt = group('VLA_VIDEO_PLATINUM_BASE')
    #base_pt.card.reqs.shards.replicas -= 3
    base_pt.card.reqs.instances.memory_guarantee.value += 10 * Gb  # Multi, quick
    base_t0 = group('VLA_VIDEO_TIER0_BASE')
    #base_t0.card.reqs.shards.replicas -= 1
    base_t0.card.reqs.instances.memory_guarantee.value += 10 * Gb  # Multi, quick
    return [
        emb_pt,
        emb_t0,
        ii_pt,
        ii_t0,
        ints_pt,
        ints_t0,
        base_pt,
        base_t0
    ]


def _vla_gencfg_4142_imgs():
    # https://st.yandex-team.ru/GENCFG-4375#5f7d8e86cd88b650f1ae6ff2
    embeddings = group('VLA_IMGS_TIER0_EMBEDDING')
    embeddings.card.reqs.disk = 10 * Gb
    embeddings.card.reqs.ssd = 160 * Gb
    invindex = group('VLA_IMGS_INVERTED_INDEX')
    t0 = group('VLA_IMGS_BASE')
    t0.card.reqs.instances.memory_guarantee.value += 20 * Gb  # [10, 15)
    t1 = group('VLA_IMGS_T1_BASE')
    t1.card.reqs.instances.memory_guarantee.value += 2 * Gb
    t0_int = group('VLA_IMGS_INT')
    t0_int.card.legacy.funcs.instanceCount = 'exactly1'
    return [
        embeddings,
        invindex,
        t0,
        t1,
        group('VLA_IMGS_INT'), group('VLA_IMGS_CBIR_INT'), group('VLA_IMGS_RECOMMEND_INT'),
        group('VLA_IMGS_T1_INT'), group('VLA_IMGS_T1_CBIR_INT'),
    ]


def vla_gencfg_4142():
    return partial_recluster(master='VLA_YT_RTC',
                             relocate={group_requirements(core.db.CURDB, g) for g in _vla_gencfg_4142_video() + _vla_gencfg_4142_imgs() + _vla_gencfg_4142_web() + _vla_gencfg_4142_pips()},
                             wipe={},
                             backgrounds=[])


def vla_gencfg_4142_video_invndex():
    return partial_recluster(master='VLA_YT_RTC',
                             relocate={
                                 group_requirements(core.db.CURDB, group('VLA_VIDEO_PLATINUM_INVERTED_INDEX')),
                                 group_requirements(core.db.CURDB, group('VLA_VIDEO_TIER0_INVERTED_INDEX')),
                             },
                             wipe={},
                             backgrounds=[])


def VLA_WEB_TIER0_BUILD():
    return partial_recluster(master='VLA_YT_RTC',
                             relocate={
                                 group_requirements(core.db.CURDB, group('VLA_WEB_TIER0_BUILD')),
                             },
                             wipe={},
                             backgrounds=[])


def man_4375_invindex():
    db = core.db.CURDB
    platinum = db.groups.get_group('MAN_VIDEO_PLATINUM_INVERTED_INDEX')
    tier0 = db.groups.get_group('MAN_VIDEO_TIER0_INVERTED_INDEX')
    imgs = db.groups.get_group('MAN_IMGS_INVERTED_INDEX')

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        platinum.card.name,
        tier0.card.name,
        imgs.card.name
    ]}
    return partial_recluster(master='MAN_WEB_BASE', relocate=reqs, wipe=set(), backgrounds=[])


def man_4375_embeddings():
    db = core.db.CURDB
    platinum = db.groups.get_group('MAN_VIDEO_PLATINUM_EMBEDDING')
    tier0 = db.groups.get_group('MAN_VIDEO_TIER0_EMBEDDING')
    imgs = db.groups.get_group('MAN_IMGS_TIER0_EMBEDDING')

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        platinum.card.name,
        tier0.card.name,
        imgs.card.name
    ]}
    return partial_recluster(master='GENCFG_4375_MAN', relocate=reqs, wipe=set(), backgrounds=[])


def vla_imgs_t1():
    db = core.db.CURDB
    base = db.groups.get_group('VLA_IMGS_T1_BASE')
    ints = db.groups.get_group('VLA_IMGS_T1_INT_MULTI')
    cbir_ints = db.groups.get_group('VLA_IMGS_T1_CBIR_INT_MULTI')

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        ints.card.name,
        cbir_ints.card.name,
    ]}
    return partial_recluster(master='VLA_YT_RTC', relocate=reqs, wipe=set(), backgrounds=[])


def man_4375_ints():
    db = core.db.CURDB

    # ints: power and disk
    platinum = db.groups.get_group('MAN_VIDEO_PLATINUM_INT')
    platinum.card.reqs.shards.min_power = 160
    platinum.card.reqs.instances.disk.value = 0  # because hdd @man is corrupted with thumbs

    tier0 = db.groups.get_group('MAN_VIDEO_TIER0_INT')
    tier0.card.reqs.shards.min_power = 160
    tier0.card.reqs.instances.disk.value = 0

    imgs = db.groups.get_group('MAN_IMGS_INT')
    imgs.card.reqs.shards.min_power = 120
    imgs.card.reqs.instances.disk.value = 0

    # bases: lower replicas; sum cohabitation reqs
    #base_pt = group('MAN_VIDEO_PLATINUM_BASE')
    #base_pt_cohab = group('MAN_VIDEO_COHABITATION_PLATINUM_BASE')
    #base_pt.card.reqs.shards.replicas = 7
    #base_pt.card.reqs.shards.min_power += base_pt_cohab.card.reqs.shards.min_power
    #base_pt.card.reqs.instances.memory_guarantee.value += base_pt_cohab.card.reqs.instances.memory_guarantee.value
    #base_pt.card.reqs.instances.ssd.value += base_pt_cohab.card.reqs.instances.ssd.value
    #base_pt.card.reqs.instances.disk.value = 0

    #base_t0 = group('MAN_VIDEO_TIER0_BASE')
    #base_t0_cohab = group('MAN_VIDEO_COHABITATION_TIER0_BASE')
    #base_t0.card.reqs.shards.replicas = 4
    #base_t0.card.reqs.shards.min_power += base_t0_cohab.card.reqs.shards.min_power
    #base_t0.card.reqs.instances.memory_guarantee.value += base_t0_cohab.card.reqs.instances.memory_guarantee.value
    #base_t0.card.reqs.instances.ssd.value += base_t0_cohab.card.reqs.instances.ssd.value
    #base_t0.card.reqs.instances.disk.value = 0

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        platinum.card.name,
        tier0.card.name,
        imgs.card.name,
        #base_pt.card.name,
        # base_t0.card.name,
    ]}
    return partial_recluster(master='MAN_WEB_BASE', relocate=reqs,
                             wipe={}, # base_pt_cohab.card, base_t0_cohab.card,
                             backgrounds=[],
                             # extra_host_donors=['GENCFG_4375_MAN']
    )


def man_invindexes():
    db = core.db.CURDB
    inv = db.groups.get_group('MAN_WEB_TIER1_INVERTED_INDEX')
    inv.card.legacy.funcs.instanceCount = 'exactly6'
    N = 1  # no superinstances
    inv.card.reqs.shards.replicas = 3 / N
    inv.card.reqs.shards.min_power = 4.0 * N  # ~0.1 cores
    inv.card.reqs.instances.memory_guarantee.value = 0.5 * N * Gb
    inv.card.reqs.instances.disk.value = 10 * N * Gb
    inv.card.reqs.instances.ssd.value = 110 * N * Gb

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
       inv.card.name,
    ]}

    return partial_recluster(master='MAN_WEB_BASE',
                             relocate=reqs,
                             wipe={},
                             backgrounds=[])


def man_gencfg_4440():
    db = core.db.CURDB
    # thumbs
    th = db.groups.get_group('MAN_IMGS_THUMB_NEW')
    th.card.reqs.instances.memory_guarantee.value = 3 * Gb  # reserve
    thl = db.groups.get_group('MAN_IMGS_LARGE_THUMB')
    thl.card.reqs.instances.memory_guarantee.value = 3 * Gb  # reserve

    ### SSD heavy users
    # web tier1
    # DO NOT recluster, just steal ssd
    tier1 = db.groups.get_group('MAN_WEB_TIER1_JUPITER_BASE')
    tier1.card.reqs.instances.ssd.value = 321 * Gb

    # remote storage
    # schedule only 0.25 tb instances, 3.5 Tb in total
    rs1 = db.groups.get_group('MAN_WEB_REMOTE_STORAGE_SLOTS_1')
    rs1.card.reqs.shards.replicas = 0  # 1162
    rs2 = db.groups.get_group('MAN_WEB_REMOTE_STORAGE_SLOTS_2')
    rs2.card.reqs.shards.replicas = 0  # 3549
    rs3 = db.groups.get_group('MAN_WEB_REMOTE_STORAGE_SLOTS_3')
    rs3.card.reqs.shards.replicas = 14000  # 1807;  3.5 Pb in total
    rs3.card.legacy.funcs.instanceCount = 'exactly6'

    # invindex
    inv = db.groups.get_group('MAN_WEB_TIER1_INVERTED_INDEX')
    inv.card.legacy.funcs.instanceCount = 'exactly6'
    N = 1  # no superinstances
    inv.card.reqs.shards.replicas = 3 / N
    inv.card.reqs.shards.min_power = 4.0 * N  # ~0.1 cores
    inv.card.reqs.instances.memory_guarantee.value = 0.5 * N * Gb
    inv.card.reqs.instances.disk.value = 10 * N * Gb
    inv.card.reqs.instances.ssd.value = 110 * N * Gb

    # lucene reqs:
    # https://st.yandex-team.ru/GENCFG-4440#5fca78a6ef711c7d76e24347
    mail = db.groups.get_group('MAN_MAIL_LUCENE')
    mail.card.legacy.funcs.instanceCount = "exactly1"
    mail.card.reqs.instances.memory_guarantee.value = 20 * Gb
    mail.card.reqs.instances.ssd.value = 800 * Gb
    mail.card.reqs.instances.disk.value = 155 * Gb
    mail.card.reqs.shards.min_power = 3.5 * 40

    disk = db.groups.get_group('MAN_DISK_LUCENE')
    disk.card.legacy.funcs.instanceCount = "exactly1"
    disk.card.reqs.instances.memory_guarantee.value = 20 * Gb
    disk.card.reqs.instances.ssd.value = 850 * Gb
    disk.card.reqs.instances.disk.value = 125 * Gb
    disk.card.reqs.shards.min_power = 5.0 * 40

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
       rs1.card.name,
       rs2.card.name,
       rs3.card.name,
       inv.card.name,
       mail.card.name,
       disk.card.name,
       th.card.name,
       thl.card.name,
    ]}
    return partial_recluster(
        master='MAN_WEB_BASE',
        relocate=reqs,
        wipe=set([group_requirements(db, db.groups.get_group('MAN_WEB_REMOTE_STORAGE_BASE'))]),
        backgrounds=[],
    )


def vla_gencfg_4453():
    db = core.db.CURDB
    keyinv = db.groups.get_group('VLA_WEB_TIER0_KEYINV')

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
       keyinv.card.name,
    ]}

    return partial_recluster(master='VLA_YT_RTC',
                             relocate=reqs,
                             wipe={}, #set([group_requirements(db, db.groups.get_group('VLA_YP_COHABITATION_TIER0_BASE'))]),
                             backgrounds=[])


def man_lucene_extra():
    db = core.db.CURDB
    disk = db.groups.get_group('MAN_DISK_LUCENE')
    disk.card.reqs.instances.ssd.value = 850 * Gb

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
       disk.card.name,
       'MAN_MAIL_LUCENE'
    ]}

    return partial_recluster(master='MAN_WEB_BASE',
                             relocate=reqs,
                             wipe={},
                             backgrounds=[])


def vla_imgs_invindex_build():
    db = core.db.CURDB
    imgs = db.groups.get_group('VLA_IMGS_INVERTED_INDEX_BUILD')
    imgs.card.reqs.shards.min_power = 1.0
    imgs.card.reqs.hosts.max_per_switch = 4

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        imgs.card.name
    ]}
    return partial_recluster(master='VLA_YT_RTC', relocate=reqs, wipe=set(), backgrounds=[])


# СХОДИТСЯ
# ждем освобождения ssd
def vla_gencfg_4457():
    db = core.db.CURDB
    wipe = set()

    # 5 cores / 20G RAM / 850G SSD / 125G HDD
    disk1 = db.groups.get_group('VLA_DISK_LUCENE')
    disk1.card.legacy.funcs.instanceCount = "exactly1"
    disk1.card.reqs.instances.memory_guarantee.value = 20 * Gb
    disk1.card.reqs.instances.ssd.value = 850 * Gb  # 850
    disk1.card.reqs.instances.disk.value = 125 * Gb  # 125
    #disk1.card.reqs.shards.min_power = 5.0 * 40

    #disk2 = db.groups.get_group('ALL_DISK_LUCENE_EXTRA_REPLICA')
    #disk2.card.legacy.funcs.instanceCount = "exactly1"
    #disk2.card.reqs.instances.memory_guarantee.value = 20 * Gb
    #disk2.card.reqs.instances.ssd.value = 850 * Gb
    #disk2.card.reqs.instances.disk.value = 125 * Gb
    # disk2.card.reqs.shards.min_power = 5.0 * 40

    # 3.5 cores / 20G RAM / 800G SSD / 155G HDD
    mail1 = db.groups.get_group('VLA_MAIL_LUCENE')
    mail1.card.legacy.funcs.instanceCount = "exactly1"
    mail1.card.reqs.instances.memory_guarantee.value = 20 * Gb
    mail1.card.reqs.instances.ssd.value = 800 * Gb  # 800
    mail1.card.reqs.instances.disk.value = 155 * Gb  # 155
    #mail1.card.reqs.shards.min_power = 3.5 * 40

    #mail2 = db.groups.get_group('ALL_MAIL_LUCENE_EXTRA_REPLICA')
    #mail2.card.legacy.funcs.instanceCount = "exactly1"
    #mail2.card.reqs.instances.memory_guarantee.value = 20 * Gb
    #mail2.card.reqs.instances.ssd.value = 800 * Gb
    #mail2.card.reqs.instances.disk.value = 155 * Gb
    #mail2.card.reqs.shards.min_power = 3.5 * 40

    # в манке 14000 * 0.25 = 3500 Pb ssd под rs
    rs = db.groups.get_group('VLA_WEB_TIER1_REMOTE_STORAGE_BASE')
    #rs.card.legacy.funcs.instanceCount = "exactly4"
    rs.card.reqs.instances.ssd.value = 700 * Gb  #512 * Gb
    #rs.card.reqs.instances.disk.value = 3 * Gb
    #rs.card.reqs.shards.replicas = 7000

    tier1 = db.groups.get_group('VLA_WEB_TIER1_JUPITER_BASE')
    #tier1_cohab = db.groups.get_group('VLA_YP_COHABITATION_TIER1_BASE')
    tier1.card.reqs.instances.ssd.value = 300 * Gb  # += tier1_cohab.card.reqs.instances.ssd.value
    #tier1.card.reqs.instances.disk.value += tier1_cohab.card.reqs.instances.disk.value
    #tier1.card.reqs.instances.memory_guarantee.value += tier1_cohab.card.reqs.instances.memory_guarantee.value
    #tier1.card.reqs.shards.min_power = 720 #+= tier1_cohab.card.reqs.shards.min_power
    #wipe.add(tier1_cohab)
    t1builder = db.groups.get_group('VLA_WEB_TIER1_BUILD')
    t1builder.card.reqs.instances.ssd.value = 200 * Gb

    #plt = db.groups.get_group('VLA_WEB_PLATINUM_JUPITER_BASE')
    #plt_cohab = db.groups.get_group('VLA_YP_COHABITATION_PLATINUM_BASE')
    #plt.card.reqs.instances.ssd.value = plt_cohab.card.reqs.instances.ssd.value
    #plt.card.reqs.instances.disk.value += plt_cohab.card.reqs.instances.disk.value
    #plt.card.reqs.instances.memory_guarantee.value += plt_cohab.card.reqs.instances.memory_guarantee.value
    #plt.card.reqs.shards.min_power += plt_cohab.card.reqs.shards.min_power
    #wipe.add(plt_cohab)

    reqs = {group_requirements(db, db.groups.get_group(group)) for group in [
        mail1.card.name,
        #mail2.card.name,
        disk1.card.name,
        #disk2.card.name,
        #rs.card.name,
        #tier1.card.name,
        #plt.card.name,
        #'VLA_WEB_PLATINUM_JUPITER_INT',
        #'VLA_WEB_CALLISTO_CAM_BASE', 'VLA_WEB_CALLISTO_CAM_INT', 'VLA_WEB_CALLISTO_CAM_INTL2',
        #'VLA_WEB_TIER1_JUPITER_INT', 'VLA_WEB_INTL2', 'VLA_WEB_TIER1_INVERTED_INDEX', # 'VLA_WEB_TIER1_EMBEDDING'
        'VLA_WEB_TIER1_INVERTED_INDEX',
    ]}
    master = db.groups.get_group('VLA_YT_RTC')

    # wipe web tier0
    wipe.update(group for group in db.groups.get_group('VLA_YT_RTC').card.slaves if 'WEB_TIER0' in group.card.name or 'VLA_YP_COHABITATION_TIER0_BASE' in group.card.name)

    # wipe builders
    for group in master.card.slaves:
        if group.card.name.endswith('VLA_WEB_TIER1_BUILD'):
            pass

    return partial_recluster(master='VLA_YT_RTC', relocate=reqs,
                             wipe=set([group_requirements(db, group) for group in wipe]), backgrounds=[])


def gencfg_4464_sas():
    return partial_recluster(master='SAS_WEB_BASE',
                             relocate={
                                 group_requirements(core.db.CURDB, group('SAS_IMGS_T1_INT_HAMSTER')),
                                 group_requirements(core.db.CURDB, group('SAS_IMGS_T1_CBIR_INT_HAMSTER')),
                             },
                             wipe={},
                             backgrounds=[])


def gencfg_4464_man():
    return partial_recluster(master='MAN_WEB_BASE',
                             relocate={
                                 group_requirements(core.db.CURDB, group('MAN_IMGS_T1_INT_HAMSTER')),
                                 group_requirements(core.db.CURDB, group('MAN_IMGS_T1_CBIR_INT_HAMSTER')),
                             },
                             wipe={},
                             backgrounds=[])


# disk -> ssd
# strenghten antiaffinity
# result: ok
def man_151_embeddings():
    pt = group('MAN_VIDEO_PLATINUM_EMBEDDING')
    pt.card.reqs.instances.ssd.value = pt.card.reqs.instances.disk.value
    pt.card.reqs.instances.disk.value = 0
    pt.card.legacy.funcs.instanceCount = 'exactly2'
    #pt.card.reqs.hosts.max_per_switch = 10
    t0 = group('MAN_VIDEO_TIER0_EMBEDDING')
    t0.card.reqs.instances.ssd.value = t0.card.reqs.instances.disk.value
    t0.card.reqs.instances.disk.value = 0
    t0.card.legacy.funcs.instanceCount = 'exactly2'
    #t0.card.reqs.hosts.max_per_switch = 10
    im = group('MAN_IMGS_TIER0_EMBEDDING')
    im.card.reqs.instances.ssd.value = im.card.reqs.instances.disk.value
    im.card.reqs.instances.disk.value = 0
    im.card.legacy.funcs.instanceCount = 'exactly2'
    #im.card.reqs.hosts.max_per_switch = 10
    return partial_recluster(master='MAN_WEB_BASE',
                             relocate={
                                 group_requirements(core.db.CURDB, pt),
                                 group_requirements(core.db.CURDB, t0),
                                 group_requirements(core.db.CURDB, im),
                             },
                             wipe={},
                             backgrounds=[])


# no changes: fail
# relax antiaffinity 1 -> 2: ok
# reserve zero resources: ok
# combine with embeddings: ok
def man_151_lucene():
    mail = group('MAN_MAIL_LUCENE')
    mail.card.legacy.funcs.instanceCount = 'exactly1'
    disk = group('MAN_DISK_LUCENE')
    disk.card.legacy.funcs.instanceCount = 'exactly1'

    # copypasted from man_151_embeddings
    pt = group('MAN_VIDEO_PLATINUM_EMBEDDING')
    #pt.card.reqs.instances.ssd.value = pt.card.reqs.instances.disk.value
    #pt.card.reqs.instances.disk.value = 0
    #pt.card.legacy.funcs.instanceCount = 'exactly1'
    #pt.card.reqs.hosts.max_per_switch = 10
    t0 = group('MAN_VIDEO_TIER0_EMBEDDING')
    #t0.card.reqs.instances.ssd.value = t0.card.reqs.instances.disk.value
    #t0.card.reqs.instances.disk.value = 0
    #t0.card.legacy.funcs.instanceCount = 'exactly1'
    #t0.card.reqs.hosts.max_per_switch = 10
    im = group('MAN_IMGS_TIER0_EMBEDDING')
    #im.card.reqs.instances.ssd.value = im.card.reqs.instances.disk.value
    #im.card.reqs.instances.disk.value = 0
    #im.card.legacy.funcs.instanceCount = 'exactly1'
    #im.card.reqs.hosts.max_per_switch = 10

    return partial_recluster(master='MAN_WEB_BASE',
                             relocate={
                                 #group_requirements(core.db.CURDB, mail),
                                 #group_requirements(core.db.CURDB, disk),
                                 group_requirements(core.db.CURDB, pt),
                                 group_requirements(core.db.CURDB, t0),
                                 group_requirements(core.db.CURDB, im),
                             },
                             wipe={},
                             backgrounds=[],
                             reserve=Resources(mem=0, ssd=0, power=0, hdd=0))


def man_152():
    # NB: mem from multi, disk
    t1_base = group('MAN_WEB_TIER1_JUPITER_BASE')
    t1_base.power = 720
    t1_base.mem += 130 * Gb
    t1_base.ssd += 35 * Gb

    # NB: mem from multi, disk
    pt_base = group('MAN_WEB_PLATINUM_JUPITER_BASE')
    pt_base.power = 400
    pt_base.mem += 16 * Gb
    pt_base.ssd += 20 * Gb

    imgs_base = group('MAN_IMGS_BASE')
    imgs_base.power = 340
    imgs_base.mem += 80 * Gb

    rs = group('MAN_WEB_REMOTE_STORAGE_SLOTS_3')
    rs.card.legacy.funcs.instanceCount = 'exactly8'

    wipe = [
        'MAN_YP_COHABITATION_TIER1_BASE',
        'MAN_YP_COHABITATION_PLATINUM_BASE',
        'MAN_IMGS_COHABITATION_T0_BASE',
        'MAN_WEB_REMOTE_STORAGE_BASE',
    ]

    # NB: reserve
    return partial_recluster(master='MAN_WEB_BASE',
                             relocate={
                                 group_requirements(core.db.CURDB, t1_base),
                                 group_requirements(core.db.CURDB, group('MAN_WEB_TIER1_EMBEDDING')),
                                 group_requirements(core.db.CURDB, pt_base),
                                 group_requirements(core.db.CURDB, imgs_base),
                                 group_requirements(core.db.CURDB, group('MAN_WEB_TIER1_JUPITER_INT')),
                                 group_requirements(core.db.CURDB, group('MAN_WEB_CALLISTO_CAM_BASE')),
                                 group_requirements(core.db.CURDB, group('MAN_WEB_CALLISTO_CAM_INT')),
                                 group_requirements(core.db.CURDB, group('MAN_WEB_CALLISTO_CAM_INTL2')),
                                 group_requirements(core.db.CURDB, rs),
                             },
                             wipe=set(group_requirements(core.db.CURDB, core.db.CURDB.groups.get_group(group)) for group in wipe),
                             backgrounds=[],
                             reserve=Resources(mem=0, ssd=0, power=0, hdd=0))


def base_with_cohabitants(db, base, relocate, wipe):
    base = group(base)
    relocate.append(base)
    for slave in base.card.master.card.slaves:
        if slave.card.host_donor == base.card.name:
            donee = group(slave.card.name)
            base.consume(donee)
            wipe.append(donee)


# relief:
#   multibetas memory
#   antiaffinity
#
# remote storage cpu experiments:
#    no cpu, full ssd - ok
#  steal all cpu from nonprod ints:
#    40 cpu, no ssd: 3500 instances
#    40 cpu, 1 Tb ssd, 4 at host: 3500 instances
#    40 cpu, 1 Tb ssd, 2 at host: 3450 instances
# Iteration 346000
# extras: 200 ok
#         300 not ok
def man_152_v3():
    db = core.db.CURDB
    wipe = []
    relocate = []
    base_with_cohabitants(db, 'MAN_WEB_TIER1_JUPITER_BASE', relocate, wipe)
    base_with_cohabitants(db, 'MAN_WEB_PLATINUM_JUPITER_BASE', relocate, wipe)
    base_with_cohabitants(db, 'MAN_IMGS_BASE', relocate, wipe)
    base_with_cohabitants(db, 'MAN_IMGS_T1_BASE', relocate, wipe)
    base_with_cohabitants(db, 'MAN_VIDEO_PLATINUM_BASE', relocate, wipe)
    base_with_cohabitants(db, 'MAN_VIDEO_TIER0_BASE', relocate, wipe)
    relocate += [
        group('MAN_WEB_TIER1_JUPITER_INT'),
        group('MAN_WEB_TIER1_JUPITER_INT_HAMSTER'),
        group('MAN_WEB_TIER1_JUPITER_INT_MULTI'),
        group('MAN_WEB_TIER1_EMBEDDING'),

        group('MAN_WEB_PLATINUM_JUPITER_INT'),
        group('MAN_WEB_PLATINUM_JUPITER_INT_HAMSTER'),
        group('MAN_WEB_PLATINUM_JUPITER_INT_MULTI'),

        group('MAN_WEB_CALLISTO_CAM_BASE'),
        group('MAN_WEB_CALLISTO_CAM_INT'),
        group('MAN_WEB_CALLISTO_CAM_INTL2'),

        group('MAN_WEB_INTL2'),
        group('MAN_WEB_INTL2_HAMSTER'),
        group('MAN_WEB_INTL2_MULTI'),

        group('MAN_WEB_GEMINI_BASE'),

        group('MAN_IMGS_CBIR_INT'),
        group('MAN_IMGS_CBIR_INT_HAMSTER'),
        group('MAN_IMGS_INT'),
        group('MAN_IMGS_INT_HAMSTER'),
        group('MAN_IMGS_INVERTED_INDEX'),
        group('MAN_IMGS_RECOMMEND_INT'),
        group('MAN_IMGS_RECOMMEND_INT_HAMSTER'),
        group('MAN_IMGS_RIM_3K'),
        group('MAN_IMGS_T1_INT'),
        group('MAN_IMGS_T1_CBIR_INT'),
        group('MAN_VIDEO_PLATINUM_INT'),
        group('MAN_VIDEO_PLATINUM_INT_HAMSTER'),
        group('MAN_VIDEO_PLATINUM_INVERTED_INDEX'),
        group('MAN_VIDEO_TIER0_INT'),
        group('MAN_VIDEO_TIER0_INT_HAMSTER'),
        group('MAN_VIDEO_TIER0_INVERTED_INDEX'),
        group('MAN_WEB_TIER1_INVERTED_INDEX'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_1'),
    ]
    wipe += [
        group('MAN_WEB_REMOTE_STORAGE_BASE'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_1'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_2'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_3')
    ]

    return partial_recluster(relocate, wipe,
                             master='MAN_WEB_BASE',
                             backgrounds=[],
                             reserve=Resources(mem=5 * Gb, ssd=0, power=0, hdd=50))


def man_152_next():
    db = core.db.CURDB
    wipe = []
    relocate = [
        group('ALL_IMGS_RQ2_BASE_PRIEMKA'),
        group('MAN_ARCNEWS_BASE'),
        group('MAN_ARCNEWS_STORY_BASE'),
        group('MAN_IMGS_COMMERCIAL_BALANCER'),
    ]

    return partial_recluster(relocate, wipe,
                             master='MAN_WEB_BASE',
                             backgrounds=[],
                             reserve=Resources(mem=5 * Gb, ssd=0, power=0, hdd=50))


def vla_attribute():
    db = core.db.CURDB
    wipe = []
    relocate = []
    base_with_cohabitants(db, 'VLA_WEB_TIER0_BASE', relocate, wipe)
    relocate += [
        group('VLA_WEB_TIER0_ATTRIBUTE_BASE'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE'),
        group('VLA_WEB_TIER0_EMBEDDING'),
        group('VLA_WEB_TIER0_INT'),
    ]

    return partial_recluster(relocate, wipe,
                             master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(mem=0 * Gb, ssd=0, power=0, hdd=0))


def webtier0_prod_vla_to_sas():
    db = core.db.CURDB
    wipe = []
    relocate = []
    base_with_cohabitants(db, 'VLA_WEB_TIER0_BASE', relocate, wipe)
    relocate += [
        group('VLA_WEB_TIER0_EMBEDDING'),
        group('VLA_WEB_TIER0_INVERTED_INDEX'),
        group('VLA_WEB_TIER0_INT'),
        group('VLA_WEB_TIER0_INTL2'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE'),
    ]
    return partial_recluster(relocate, wipe, master='SAS_WEB_BASE', backgrounds=[])


def webtier0_prod_sas():
    db = core.db.CURDB
    wipe = []
    relocate = []
    relocate += [
        group('SAS_WEB_TIER0_BASE'),
        group('SAS_WEB_TIER0_ATTRIBUTE_BASE'),
        group('SAS_WEB_TIER0_EMBEDDING'),
        group('SAS_WEB_TIER0_INVERTED_INDEX'),
        group('SAS_WEB_TIER0_INT'),
        group('SAS_WEB_TIER0_INTL2'),
        group('SAS_WEB_TIER0_REMOTE_STORAGE_BASE'),
    ]
    return partial_recluster(relocate, wipe, master='SAS_WEB_BASE', backgrounds=[])


def man_153_rs():
    db = core.db.CURDB
    wipe = [
        group('MAN_WEB_REMOTE_STORAGE_BASE'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_1'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_2'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_3'),
    ]
    relocate = [
        group('MAN_WEB_TIER1_INVERTED_INDEX'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_3'),
    ]

    rs3 = group('MAN_WEB_REMOTE_STORAGE_SLOTS_3')
    rs3.card.reqs.shards.replicas = 13750
    return partial_recluster(relocate, wipe,
                             master='MAN_WEB_BASE',
                             backgrounds=[],
                             reserve=Resources(mem=5 * Gb, ssd=0, power=40, hdd=50))


def sas_single_rim():
    db = core.db.CURDB
    wipe = []
    rim = group('SAS_IMGS_RIM_3K')
    rim.power = 4
    relocate = [
        rim
    ]
    return partial_recluster(relocate, wipe, master='SAS_WEB_BASE', backgrounds=[], reserve=Resources(mem=1 * Gb, ssd=0, power=0, hdd=0))


def enlarge_invindex():
    # >>> 1949 * 1.0 + 405 * 0.25 + 1073 * 1.25
    # 3391.5

    db = core.db.CURDB
    wipe = [
        # group('SAS_WEB_REMOTE_STORAGE_BASE'),
        # group('SAS_WEB_REMOTE_STORAGE_BASE_SLOTS'),
        # group('SAS_WEB_REMOTE_STORAGE_BASE_SLOTS_2'),
        # group('SAS_WEB_REMOTE_STORAGE_BASE_SLOTS_3'),
    ]
    #t1 = group('SAS_YP_COHABITATION_TIER1_BASE')
    #t1.ssd = gb(600)  # 840
    invindex = group('VLA_WEB_TIER1_INVERTED_INDEX')
    invindex.ssd = 155 * Gb
    invindex_pip = group('VLA_WEB_TIER1_INVERTED_INDEX_PIP')
    invindex_pip.ssd = 155 * Gb
    #invindex.power = 5
    # rs = group('SAS_WEB_REMOTE_STORAGE_BASE_SLOTS')
    # rs.ssd = gb(256)
    # rs.card.reqs.shards.replicas = 13600
    # rs.card.legacy.funcs.instanceCount = "exactly8"
    relocate = [
        invindex,
        invindex_pip,
        # rs
    ]
    return partial_recluster(relocate, wipe, master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(power=0, mem=gb(3), ssd=0, hdd=50))


def vla_154():
    db = core.db.CURDB
    wipe = [
        group('VLA_WEB_TIER0_BASE'),
        group('VLA_WEB_TIER0_EMBEDDING'),
        group('VLA_WEB_TIER0_INT'),
        group('VLA_WEB_TIER0_INTL2'),
        group('VLA_WEB_TIER0_INVERTED_INDEX'),
        group('VLA_WEB_TIER0_KEYINV'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE'),
        group('VLA_YP_COHABITATION_TIER0_BASE'),
    ]
    relocate = []
    base_with_cohabitants(db, 'VLA_WEB_TIER1_JUPITER_BASE', relocate, wipe)
    base_with_cohabitants(db, 'VLA_WEB_PLATINUM_JUPITER_BASE', relocate, wipe)
    base_with_cohabitants(db, 'VLA_VIDEO_PLATINUM_BASE', relocate, wipe)
    base_with_cohabitants(db, 'VLA_VIDEO_TIER0_BASE', relocate, wipe)
    base_with_cohabitants(db, 'VLA_IMGS_BASE', relocate, wipe)
    base_with_cohabitants(db, 'VLA_IMGS_T1_BASE', relocate, wipe)

    webinv = group('VLA_WEB_TIER1_INVERTED_INDEX')
    webinv.ssd = 155 * Gb
    webinvpip = group('VLA_WEB_TIER1_INVERTED_INDEX_PIP')
    webinvpip.ssd = 155 * Gb
    webt0invpip = group('VLA_WEB_TIER0_INVERTED_INDEX_PIP')
    webt0invpip.ssd = 90 * Gb

    rs = group('VLA_WEB_TIER1_REMOTE_STORAGE_BASE')
    rs.card.reqs.shards.replicas = 4000
    #rs.mem = 1.5 * Gb
    rs_pip = group('VLA_WEB_TIER1_REMOTE_STORAGE_BASE_PIP')
    rs_pip.card.reqs.shards.replicas = 4000

    relocate += [
        # web
        group('VLA_WEB_INTL2'),
        group('VLA_WEB_TIER1_JUPITER_INT'),
        group('VLA_WEB_TIER1_EMBEDDING'),
        group('VLA_WEB_PLATINUM_JUPITER_INT'),
        webinv,
        rs,

        # web pip
        webinvpip,
        webt0invpip,
        rs_pip,

        # callisto
        # !increase mem for hamster when reclustering callisto base
        # group('VLA_WEB_CALLISTO_CAM_BASE'),
        # group('VLA_WEB_CALLISTO_CAM_INT'),
        # group('VLA_WEB_CALLISTO_CAM_INTL2'),

        # video
        group('VLA_VIDEO_PLATINUM_EMBEDDING'),
        group('VLA_VIDEO_PLATINUM_INVERTED_INDEX'),
        group('VLA_VIDEO_PLATINUM_INT'),
        group('VLA_VIDEO_TIER0_EMBEDDING'),
        group('VLA_VIDEO_TIER0_INVERTED_INDEX'),
        group('VLA_VIDEO_TIER0_INT'),
        group('VLA_VIDEO_REMOTE_STORAGE_BASE'),
        group('VLA_VIDEO_REMOTE_STORAGE_BASE_PIP'),

        # imgs
        group('VLA_IMGS_INT'),
        group('VLA_IMGS_TIER0_EMBEDDING'),
        group('VLA_IMGS_INVERTED_INDEX'),
        group('VLA_IMGS_T1_INT'),
    ]

    return partial_recluster(relocate, wipe,
                             master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(mem=2 * Gb, ssd=0, power=0, hdd=0))


def vla_tier0_pip():
    db = core.db.CURDB
    tier0prod = [
        group('VLA_WEB_TIER0_BASE'),
        group('VLA_WEB_TIER0_ATTRIBUTE_BASE'),
        group('VLA_WEB_TIER0_EMBEDDING'),
        group('VLA_WEB_TIER0_KEYINV'),
        group('VLA_WEB_TIER0_INVERTED_INDEX'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_1'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE'),
        group('VLA_WEB_TIER0_INT'),
        group('VLA_WEB_TIER0_INTL2'),
    ]
    tier0pip = [
        #group('VLA_WEB_TIER0_KEYINV_PIP'),
        #group('VLA_WEB_TIER0_EMBEDDING_PIP'),
        #group('VLA_WEB_TIER0_INVERTED_INDEX_PIP'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_PIP_SLOTS_1'),
    ]
    for g in tier0prod + tier0pip:
        # g.ssd = 0
        # g.power = 0
        # g.mem = 0
        pass

    relocate = [
        # tier0 pip
        group('VLA_WEB_TIER0_ATTRIBUTE_BASE_PIP'),
        group('VLA_WEB_TIER0_BASE_PIP'),
        group('VLA_WEB_TIER0_KEYINV_PIP'),
        group('VLA_WEB_TIER0_EMBEDDING_PIP'),
        group('VLA_WEB_TIER0_INVERTED_INDEX_PIP'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_PIP_SLOTS_1'),
        # group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE_PIP'),
        # later
        # group('VLA_MOTHER_BUILD'),
    ]

    return partial_recluster(relocate, wipe=[],
                             master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(mem=2 * Gb, ssd=0, power=0, hdd=0))


def vla_tier0_prod():
    db = core.db.CURDB
    tier1 = [
        group('VLA_WEB_TIER1_JUPITER_BASE'),
        group('VLA_WEB_TIER1_EMBEDDING'),
        group('VLA_WEB_TIER1_REMOTE_STORAGE_BASE'),
        group('VLA_WEB_TIER1_INVERTED_INDEX'),
        group('VLA_WEB_TIER1_JUPITER_INT'),
        group('VLA_WEB_INTL2'),
    ]
    # drop cpu and mem, leave ssd
    for g in tier1:
        # g.mem = 0
        # g.power = 0
        pass

    relocate = [
        # tier0 prod
        group('VLA_WEB_TIER0_BASE'),
        group('VLA_WEB_TIER0_ATTRIBUTE_BASE'),
        group('VLA_WEB_TIER0_EMBEDDING'),
        group('VLA_WEB_TIER0_KEYINV'),
        group('VLA_WEB_TIER0_INVERTED_INDEX'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_1'),
        group('VLA_WEB_TIER0_INT'),
        group('VLA_WEB_TIER0_INTL2'),
    ]

    return partial_recluster(relocate, wipe=[],
                             master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(mem=2 * Gb, ssd=0, power=0, hdd=0))


# converges with callisto and platinum
# and 8 slots per host (of 256 gb)
def man_tier0_prod():
    db = core.db.CURDB
    tier1 = [
        group('MAN_WEB_INTL2_ITDITP'),
        group('MAN_WEB_INTL2'),
        group('MAN_WEB_INTL2_HAMSTER'),
        group('MAN_WEB_INTL2_MULTI'),
        group('MAN_WEB_TIER1_JUPITER_INT'),
        group('MAN_WEB_TIER1_JUPITER_INT_HAMSTER'),
        group('MAN_WEB_TIER1_JUPITER_INT_ITDITP'),
        group('MAN_WEB_TIER1_JUPITER_INT_MULTI'),
        group('MAN_WEB_TIER1_JUPITER_BASE'),
        group('MAN_WEB_TIER1_JUPITER_BASE_HAMSTER'),
        group('MAN_WEB_TIER1_JUPITER_BASE_ITDITP'),
        group('MAN_WEB_TIER1_JUPITER_BASE_MULTI'),
        group('MAN_WEB_TIER1_EMBEDDING'),
        group('MAN_WEB_TIER1_INVERTED_INDEX'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_1'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_2'),
        group('MAN_WEB_REMOTE_STORAGE_SLOTS_3'),
        group('MAN_WEB_REMOTE_STORAGE_BASE'),
    ]

    slots = group('MAN_WEB_TIER0_REMOTE_STORAGE_SLOTS_1')
    slots.card.reqs.shards.replicas = 14500  # 14366 = 3.5 Pb / 256 Gb per slot.
                                             # 3567.77 Tb нужно для рс-ов т0, проверялось во владимире
    dmem = 52 * Gb
    base = group('MAN_WEB_TIER0_BASE')
    base.mem += dmem
    attr = group('MAN_WEB_TIER0_ATTRIBUTE_BASE')
    attr.mem += dmem

    relocate = [
        # tier0
        attr,
        base,
        group('MAN_WEB_TIER0_EMBEDDING'),
        group('MAN_WEB_TIER0_INT'),
        group('MAN_WEB_TIER0_INT_HAMSTER'),
        group('MAN_WEB_TIER0_INT_MULTI'),
        group('MAN_WEB_TIER0_INTL2'),
        group('MAN_WEB_TIER0_INTL2_HAMSTER'),
        group('MAN_WEB_TIER0_INTL2_MULTI'),
        group('MAN_WEB_TIER0_INVERTED_INDEX'),
        group('MAN_WEB_TIER0_KEYINV'),
        slots,

        # platinum
        group('MAN_WEB_PLATINUM_JUPITER_BASE'),
        group('MAN_WEB_PLATINUM_JUPITER_INT'),
        group('MAN_WEB_PLATINUM_JUPITER_INT_HAMSTER'),
        group('MAN_WEB_PLATINUM_JUPITER_INT_MULTI'),
        group('MAN_WEB_PLATINUM_JUPITER_INT_ITDITP'),

        # callisto
        group('MAN_WEB_CALLISTO_CAM_BASE'),
        group('MAN_WEB_CALLISTO_CAM_INT'),
        group('MAN_WEB_CALLISTO_CAM_INTL2'),
    ]

    return partial_recluster(relocate, wipe=tier1,
                             master='MAN_WEB_BASE',
                             backgrounds=[],
                             reserve=Resources(mem=5 * Gb, ssd=0, power=40, hdd=50))


def vla_tier0_hamster():
    db = core.db.CURDB
    ints = group('VLA_WEB_TIER0_INT_MULTI')
    ints.card.reqs.shards.replicas = 1440
    intl2 = group('VLA_WEB_TIER0_INTL2_MULTI')

    relocate = [
        ints,
        intl2
    ]

    return partial_recluster(relocate, wipe=[],
                             master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(mem=2 * Gb, ssd=0, power=0, hdd=0))


def sas_tier0_prod():
    db = core.db.CURDB
    tier1 = [
        group('SAS_WEB_INTL2_ITDITP'),
        group('SAS_WEB_INTL2'),
        group('SAS_WEB_INTL2_HAMSTER'),
        # group('SAS_WEB_INTL2_MULTI'),
        group('SAS_WEB_TIER1_JUPITER_INT'),
        group('SAS_WEB_TIER1_JUPITER_INT_HAMSTER'),
        group('SAS_WEB_TIER1_JUPITER_INT_ITDITP'),
        # group('SAS_WEB_TIER1_JUPITER_INT_MULTI'),
        group('SAS_WEB_TIER1_JUPITER_BASE'),
        group('SAS_WEB_TIER1_JUPITER_BASE_HAMSTER'),
        group('SAS_WEB_TIER1_JUPITER_BASE_ITDITP'),
        # group('SAS_WEB_TIER1_JUPITER_BASE_MULTI'),
        group('SAS_WEB_TIER1_EMBEDDING'),
        group('SAS_WEB_TIER1_INVERTED_INDEX'),
        group('SAS_WEB_REMOTE_STORAGE_BASE_SLOTS'),
        group('SAS_WEB_REMOTE_STORAGE_BASE_SLOTS_2'),
        group('SAS_WEB_REMOTE_STORAGE_BASE_SLOTS_3'),
        group('SAS_WEB_REMOTE_STORAGE_BASE'),
    ]

    slots = group('SAS_WEB_TIER0_REMOTE_STORAGE_SLOTS_1')
    slots.card.reqs.shards.replicas = 3050  # int(3.5 * 1024)  # 3.5 Pb / 0.5 Tb per slot.
                                            # 3567.77 Tb нужно для рс-ов т0, проверялось во владимире
    dmem = 0 * Gb
    base = group('SAS_WEB_TIER0_BASE')
    base.mem += dmem
    attr = group('SAS_WEB_TIER0_ATTRIBUTE_BASE')
    attr.mem += dmem
    embedding = group('SAS_WEB_TIER0_EMBEDDING')

    relocate = [
        # tier0
        attr,
        base,
        slots,
        embedding,
        group('SAS_WEB_TIER0_INT'),
        group('SAS_WEB_TIER0_INT_HAMSTER'),
        group('SAS_WEB_TIER0_INT_MULTI'),
        group('SAS_WEB_TIER0_INTL2'),
        group('SAS_WEB_TIER0_INTL2_HAMSTER'),
        group('SAS_WEB_TIER0_INTL2_MULTI'),
        group('SAS_WEB_TIER0_INVERTED_INDEX'),
        group('SAS_WEB_TIER0_KEYINV'),

        # callisto
        # group('SAS_WEB_CALLISTO_CAM_BASE'),
        # group('SAS_WEB_CALLISTO_CAM_INT'),
        # group('SAS_WEB_CALLISTO_CAM_INTL2'),
    ]

    return partial_recluster(relocate, wipe=tier1,
                             master='SAS_WEB_BASE',
                             backgrounds=[],
                             reserve=Resources(mem=5 * Gb, ssd=0, power=40, hdd=50))

# сходится
# остается 316 overcommited by ssd, можно попробовать это починить
def vla_157_converges():
    db = core.db.CURDB

    disk = group('ALL_DISK_LUCENE_EXTRA_REPLICA')
    disk.power = 5.0 * 40
    mail = group('ALL_MAIL_LUCENE_EXTRA_REPLICA')
    mail.power = 3.5 * 40
    wipe=[
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_0'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_1'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_2'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_PIP_SLOTS_1'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE_PIP'),
    ]
    relocate = [
        group('VLA_WEB_TIER0_INVERTED_INDEX'),
        #group('VLA_WEB_TIER0_BASE'),
        #group('VLA_WEB_TIER0_ATTRIBUTE_BASE'),
        #group('VLA_WEB_TIER0_EMBEDDING'),
        #group('VLA_WEB_TIER0_KEYINV'),
        #group('VLA_WEB_TIER0_INT'),
        #group('VLA_WEB_TIER0_INTL2'),
        disk,
        mail
    ]

    return partial_recluster(relocate, wipe=wipe,
                             master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(mem=0 * Gb, ssd=0, power=0, hdd=0))

# остается 243 overcommited by ssd
def vla_157():
    db = core.db.CURDB

    disk = group('ALL_DISK_LUCENE_EXTRA_REPLICA')
    disk.power = 5.0 * 40
    mail = group('ALL_MAIL_LUCENE_EXTRA_REPLICA')
    mail.power = 3.5 * 40
    wipe=[
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_0'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_1'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_2'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE'),
        #group('VLA_WEB_TIER0_REMOTE_STORAGE_PIP_SLOTS_1'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_BASE_PIP'),
    ]
    relocate = [
        #group('VLA_WEB_TIER0_INVERTED_INDEX'),
        # group('VLA_WEB_TIER0_BASE'),
        #group('VLA_WEB_TIER0_ATTRIBUTE_BASE'),
        #group('VLA_WEB_TIER0_EMBEDDING'),
        #group('VLA_WEB_TIER0_KEYINV'),
        #group('VLA_WEB_TIER0_INT'),
        #group('VLA_WEB_TIER0_INTL2'),
        group('VLA_WEB_TIER0_REMOTE_STORAGE_PIP_SLOTS_1'),
        disk,
        mail
    ]

    return partial_recluster(relocate, wipe=wipe,
                             master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(mem=4 * Gb, ssd=0, power=0, hdd=0))

def sas_platinum():
    db = core.db.CURDB
    platinum = group('SAS_WEB_PLATINUM_JUPITER_BASE')
    platinum.card.legacy.funcs.instanceCount = 'exactly2'
    wipe = [
    ]
    relocate = [
        platinum,
        group('SAS_WEB_PLATINUM_JUPITER_INT'),
        group('SAS_WEB_PLATINUM_JUPITER_INT_HAMSTER'),
    ]
    return partial_recluster(relocate, wipe=wipe,
                             master='SAS_WEB_BASE',
                             backgrounds=[],
                             reserve=Resources(mem=6 * Gb, ssd=0, power=0, hdd=0))


def man_158():
    db = core.db.CURDB

    slots = group('MAN_WEB_TIER0_REMOTE_STORAGE_SLOTS_1')
    slots.card.reqs.shards.replicas = 14500  # 14366 = 3.5 Pb / 256 Gb per slot.
                                             # 3567.77 Tb нужно для рс-ов т0, проверялось во владимире
    relocate = [
        slots
    ]

    return partial_recluster(relocate, wipe=[],
                             master='MAN_WEB_BASE',
                             backgrounds=[],
                             reserve=Resources(mem=0 * Gb, ssd=0, power=0, hdd=0))


def vla_158():
    db = core.db.CURDB

    prod_slots_3 = group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_3')
    prod_slots_3.card.reqs.shards.replicas = 3500
    prod_slots_4 = group('VLA_WEB_TIER0_REMOTE_STORAGE_SLOTS_4')
    prod_slots_4.card.reqs.shards.replicas = 1000

    pip_slots_1 = group('VLA_WEB_TIER0_REMOTE_STORAGE_PIP_SLOTS_1')
    pip_slots_1.card.reqs.shards.replicas = 4000

    relocate = [
        prod_slots_3,
        prod_slots_4,
        pip_slots_1,
    ]

    # todo: add backgrounders
    return partial_recluster(relocate, wipe=[],
                             master='VLA_YT_RTC',
                             backgrounds=[],
                             reserve=Resources(mem=4 * Gb, ssd=0, power=0, hdd=0))


def main(fn):
    with open(fn + '.json', 'w') as f:
        json.dump(sorted(globals()[fn]()), f, indent=4, sort_keys=True)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] %(message)s')

    main(sys.argv[1])
