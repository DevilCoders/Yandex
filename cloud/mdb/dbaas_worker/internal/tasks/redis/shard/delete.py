"""
Redis shard delete executor.
"""

from ...common.shard.delete import ShardDeleteExecutor
from ...utils import build_host_group, register_executor

CLUSTER_DELETE_SHARD_SLS = 'components.redis.cluster.remove_shard'


@register_executor('redis_shard_delete')
class RedisShardDeleteExecuter(ShardDeleteExecutor):
    """
    Delete Redis shard.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.redis

    def run(self):
        """
        1) Rebalance slots from the shard master
        2) Delete shard nodes from cluster
        3) Delete shard hosts
        """
        lock_hosts = list(self.args['hosts'])
        lock_hosts.extend([host['fqdn'] for host in self.args['shard_hosts']])
        self.mlock.lock_cluster(sorted(lock_hosts))
        target_hosts = [h['fqdn'] for h in self.args['shard_hosts']]
        any_host = list(self.args['hosts'].keys())[0]

        delete_shard = [
            self._run_sls_host(
                any_host,
                CLUSTER_DELETE_SHARD_SLS,
                pillar={'deleted_hosts': target_hosts},
                environment=self.args['hosts'][any_host]['environment'],
            ),
        ]
        self.deploy_api.wait(delete_shard)
        host_group = build_host_group(self.config.redis, self.args['hosts'])
        self._run_operation_host_group(host_group, 'metadata')
        self._delete_hosts()
        self.mlock.unlock_cluster()
