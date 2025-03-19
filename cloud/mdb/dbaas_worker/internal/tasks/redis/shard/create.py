"""
Redis shard create executor.
"""
from copy import deepcopy

from ....utils import split_host_map
from ...common.shard.create import ShardCreateExecutor
from ...utils import build_host_group, register_executor


@register_executor('redis_shard_create')
class RedisShardCreateExecutor(ShardCreateExecutor):
    """
    Create Redis shard.
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        shard_id = self.args['shard_id']
        new_hosts, old_hosts = split_host_map(self.args['hosts'], shard_id)

        old_hosts_group = build_host_group(self.config.redis, old_hosts)
        new_hosts_group = build_host_group(self.config.redis, deepcopy(new_hosts))

        master_fqdn, master_opts = new_hosts.popitem()
        new_master = {master_fqdn: master_opts}
        new_slaves = new_hosts

        new_master_group = build_host_group(self.config.redis, new_master)
        new_slaves_group = build_host_group(self.config.redis, new_slaves)
        for opts in new_master_group.hosts.values():
            opts['deploy'] = {'pillar': {'new_master': True}}
        for opts in new_slaves_group.hosts.values():
            opts['deploy'] = {'pillar': {'replica': True}}

        self._health_host_group(old_hosts_group)

        self._create_host_secrets(new_hosts_group)
        self._create_host_group(new_hosts_group, revertable=True)
        self._issue_tls(new_hosts_group)
        self._run_operation_host_group(old_hosts_group, 'metadata')
        self._highstate_host_group(new_master_group)
        self._highstate_host_group(new_slaves_group)
        self._create_public_records(new_slaves_group)

        self._enable_monitoring(new_hosts_group)
        self.mlock.unlock_cluster()
