import gaux.gpu as gpu
from collections import defaultdict
from math import ceil

from utils.api.searcherlookup_groups_instances import generate_cpu_limits


def get_host_cores(groups):
    host_cores = defaultdict(float)

    for group in groups:
        for instance in group.get_kinda_busy_instances():
            host_cores[instance.host] += generate_cpu_limits(group, instance)['cpu_cores_guarantee']

    return {host: int(ceil(cores)) for host, cores in host_cores.iteritems()}


def get_instances_cores(group):
    instances_cores = defaultdict(float)

    for instance in group.get_kinda_busy_instances():
        instances_cores[instance] = generate_cpu_limits(group, instance)['cpu_cores_guarantee']

    return {instance: int(ceil(cores)) for instance, cores in instances_cores.iteritems()}


def get_instance_replicas(group):
    instance_replicas = {}

    for intlookup in (group.parent.db.intlookups.get_intlookup(x) for x in group.card.intlookups):
        for shard_id in xrange(intlookup.get_shards_count()):
            replica_instances = intlookup.get_base_instances_for_shard(shard_id)
            replica_instances_names = [instance.name() for instance in sorted(replica_instances)]
            for instance in replica_instances:
                instance_replicas[instance] = replica_instances_names

    return instance_replicas


def get_shard_numbers(group):
    shards = {}

    for intlookup in (group.parent.db.intlookups.get_intlookup(x) for x in group.card.intlookups):
        for shard_id in xrange(intlookup.get_shards_count()):
            for instance in intlookup.get_base_instances_for_shard(shard_id):
                shards[instance] = shard_id

    return shards


def cpu_sharing_groups(db, group):
    groups = {group}

    if hasattr(group.card.configs, 'supermind') and hasattr(group.card.configs.supermind, 'groups'):
        for extra_group in group.card.configs.supermind.groups:
            groups.add(db.groups.get_group(extra_group))

    return groups


def get_group_workloads(group):
    return group.card.workloads if 'workloads' in group.card else []


def get_gpu_devices(group):
    return {instance: gpu.device(group, instance) for instance in group.get_kinda_busy_instances()}


def get_template_data(group):
    return {
        'current_tag': group.parent.db.get_repo().get_current_tag() or 'trunk',
        'CURDB': group.parent.db,
        'intlookups': group.parent.db.intlookups,
        'instance_replicas': get_instance_replicas(group),
        'host_cores': get_host_cores(cpu_sharing_groups(group.parent.db, group)),
        'instances_cores': get_instances_cores(group),
        'workloads': get_group_workloads(group),
        'gpu_devices': get_gpu_devices(group),
    }
