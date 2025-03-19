# -*- coding: utf-8 -*-
"""
Redis state for salt
"""

from __future__ import absolute_import, print_function, unicode_literals

import traceback

# For arcadia tests, populate __opts__ and __salt__ variables
__opts__ = {}
__salt__ = {}


def __virtual__():
    return True


def salt_state(func):
    """
    Handles all the salt-related logic.
    """

    def wrapper(name, *args, **kwargs):
        ret = {
            'name': name,
            'result': None,
            'comment': '',
            'changes': {},
        }
        try:
            changes = func(name, *args, **kwargs)
        except Exception:
            ret['result'] = False
            ret['comment'] = traceback.format_exc()
            return ret

        ret['changes'] = changes

        if __opts__['test'] is True and changes:
            ret['result'] = None
        else:
            ret['result'] = True
        return ret

    return wrapper


@salt_state
def set_options(name, pillar_path='data:redis:config', options=None):
    """
    Set options from k/v pairs in pillar_path
    """
    opts = __salt__['pillar.get'](pillar_path, {})
    if options:
        opts.update(options)

    password = opts.get('requirepass', None)

    return __salt__['mdb_redis.set_options'](options=opts, password=password, test=(__opts__['test'] is True))


@salt_state
def rewrite_aof(name, pillar_path='data:redis:config'):
    """
    Start AOF rewrite process.
    """
    opts = __salt__['pillar.get'](pillar_path, {})

    password = opts.get('requirepass', None)

    return __salt__['mdb_redis.rewrite_aof'](password=password, test=(__opts__['test'] is True))


@salt_state
def ensure_not_master(name, host, timeout=15):
    """
    Ensure that the host is not a master
    """
    dbaas = __salt__['pillar.get']('data:dbaas', {})
    hosts = dbaas.get('shard_hosts', [])
    master_name = dbaas.get('cluster_name', 'mymaster')

    return __salt__['mdb_redis.ensure_not_master'](
        host=host, hosts=hosts, master_name=master_name, test=(__opts__['test'] is True), timeout=timeout
    )


@salt_state
def start_failover(name):
    """
    Initiate manual failover
    """
    dbaas = __salt__['pillar.get']('data:dbaas', {})
    master_name = dbaas.get('cluster_name', 'mymaster')

    return __salt__['mdb_redis.start_failover'](master_name=master_name, test=(__opts__['test'] is True))


@salt_state
def reset_sentinel(name):
    """
    Reset sentinel configuration.
    """
    dbaas = __salt__['pillar.get']('data:dbaas', {})
    master_name = dbaas.get('cluster_name', 'mymaster')

    return __salt__['mdb_redis.reset_sentinel'](master_name=master_name, test=(__opts__['test'] is True))


@salt_state
def init_cluster(name, pillar_path='data:redis:config'):
    """
    Init Redis cluster.
    """
    subclusters = __salt__['pillar.get']('data:dbaas:cluster:subclusters', {})
    opts = __salt__['pillar.get'](pillar_path, {})
    password = opts.get('requirepass', None)

    subname, subcluster = list(subclusters.items()).pop()
    shards = subcluster['shards']

    return __salt__['mdb_redis.init_cluster'](shards=shards, password=password, test=(__opts__['test'] is True))


@salt_state
def add_cluster_node(name, host, is_master=False, pillar_path='data:redis:config'):
    """
    Add cluster node to the shard to which it belongs.
    """
    subclusters = __salt__['pillar.get']('data:dbaas:cluster:subclusters', {})
    opts = __salt__['pillar.get'](pillar_path, {})
    password = opts.get('requirepass', None)

    if is_master:
        cluster_hosts = __salt__['pillar.get']('data:dbaas:cluster_hosts', [])
        shard_hosts = __salt__['pillar.get']('data:dbaas:shard_hosts', [])
        old_hosts = __salt__['mdb_redis.filter_alive_hosts']([h for h in cluster_hosts if h not in shard_hosts])
        old_host = old_hosts[0]
        return __salt__['mdb_redis.add_cluster_node'](
            host=host, password=password, old_host=old_host, test=(__opts__['test'] is True)
        )

    _, subcluster = list(subclusters.items()).pop()
    shards = subcluster['shards']
    for shard in shards.values():
        shard_hosts = list(shard.get('hosts', {}).keys())
        if host in shard_hosts:
            shard_hosts.remove(host)
            return __salt__['mdb_redis.add_cluster_node'](
                host=host, password=password, shard_hosts=shard_hosts, test=(__opts__['test'] is True)
            )
    raise RuntimeError('Failed to find host "{}" in pillar'.format(host))


@salt_state
def delete_cluster_node(name, host, pillar_path='data:redis:config'):
    """
    Delete cluster node from the shard.
    """
    opts = __salt__['pillar.get'](pillar_path, {})
    password = opts.get('requirepass', None)

    return __salt__['mdb_redis.delete_cluster_node'](host=host, password=password, test=(__opts__['test'] is True))


@salt_state
def rebalance_slots(name, pillar_path='data:redis:config'):
    """
    Rebalance cluster slot distribution.
    """
    opts = __salt__['pillar.get'](pillar_path, {})
    password = opts.get('requirepass', None)

    return __salt__['mdb_redis.rebalance_slots'](password=password, test=(__opts__['test'] is True))


@salt_state
def remove_shard(name, shard_hosts, pillar_path='data:redis:config'):
    """
    Remove shard from the cluster.
    """
    opts = __salt__['pillar.get'](pillar_path, {})
    password = opts.get('requirepass', None)

    return __salt__['mdb_redis.remove_shard'](
        shard_hosts=shard_hosts, password=password, test=(__opts__['test'] is True)
    )
