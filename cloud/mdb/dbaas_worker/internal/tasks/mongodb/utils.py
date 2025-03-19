# -*- coding: utf-8 -*-
"""
Utilities specific for MongoDB clusters.
"""

from collections import defaultdict

from ...tasks.utils import build_host_group
from ...utils import get_first_value

MONGOD_HOST_TYPE = 'mongod'
MONGOS_HOST_TYPE = 'mongos'
MONGOCFG_HOST_TYPE = 'mongocfg'
MONGOINFRA_HOST_TYPE = 'mongoinfra'
SHARDING_INFRA_HOST_TYPES = [MONGOCFG_HOST_TYPE, MONGOINFRA_HOST_TYPE, MONGOS_HOST_TYPE]
ORDERED_HOST_TYPES = [MONGOD_HOST_TYPE, MONGOCFG_HOST_TYPE, MONGOINFRA_HOST_TYPE, MONGOS_HOST_TYPE]

HOST_TYPE_ROLE = {
    MONGOD_HOST_TYPE: 'mongodb_cluster.mongod',
    MONGOS_HOST_TYPE: 'mongodb_cluster.mongos',
    MONGOCFG_HOST_TYPE: 'mongodb_cluster.mongocfg',
    MONGOINFRA_HOST_TYPE: 'mongodb_cluster.mongoinfra',
}

ROLE_HOST_TYPE = {v: k for k, v in HOST_TYPE_ROLE.items()}

TASK_CREATE_CLUSTER = 'mongodb_cluster_create'
TASK_RESTORE_CLUSTER = 'mongodb_cluster_restore'


def classify_host_map(hosts):
    """
    Classify dict of hosts.
    """
    classified_hosts = defaultdict(dict)

    for host, opts in hosts.items():
        for subcluster, role in HOST_TYPE_ROLE.items():
            if 'roles' not in opts or role in opts['roles']:
                classified_hosts[subcluster][host] = opts

    return classified_hosts


def build_new_replset_hostgroup(hosts, config, task, task_args, conductor_group_id=None):
    """
    Build new hostgroup
    """
    if conductor_group_id is None:
        conductor_group_id = get_first_value(hosts)['subcid']
    new_host_group = build_host_group(config, hosts)
    new_host_group.properties.conductor_group_id = conductor_group_id

    host_list = sorted(list(new_host_group.hosts.keys()))
    master = host_list[0]
    replicas = host_list[1:]

    master_pillar = make_master_pillar(task_type=task['task_type'], args=task_args)
    replica_pillar = make_replica_pillar(task_type=task['task_type'], args=task_args)

    new_host_group.hosts[master]['deploy'] = {
        'pillar': master_pillar,
        'title': 'master',
    }
    for replica in replicas:
        new_host_group.hosts[replica]['deploy'] = {
            'pillar': replica_pillar,
            'title': 'replica',
        }
    return new_host_group


def make_master_pillar(task_type: str, args: dict) -> dict:
    """
    Return pillar for master
    """
    pillar = {'do-backup': True}
    if 'target-pillar-id' in args:
        pillar['target-pillar-id'] = args['target-pillar-id']
    if task_type == TASK_RESTORE_CLUSTER:
        pillar['restore-from'] = args['restore-from']
    return pillar


def make_replica_pillar(task_type: str, args: dict) -> dict:
    """
    Return pillar for replica
    """
    pillar = {'replica': True}
    if task_type == TASK_RESTORE_CLUSTER:
        if 'target-pillar-id' in args:
            pillar['target-pillar-id'] = args['target-pillar-id']
    return pillar
