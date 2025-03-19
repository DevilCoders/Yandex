"""
Redis Delete host executor
"""

from ...common.host.delete import HostDeleteExecutor
from ...utils import build_host_group, register_executor

CLUSTER_DELETE_NODE_SLS = 'components.redis.cluster.delete_node'
SERVER_STEP_DOWN_SLS = 'components.redis.server.step_down'


@register_executor('redis_host_delete')
class RedisHostDelete(HostDeleteExecutor):
    """
    Delete Redis host
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.redis

    def run(self):
        """
        1) Switch master if it's on the target host.
        2) Remove the target host.
        3) Reset Sentinel configs on remaining hosts.
        """
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))
        remaining_hosts = build_host_group(self.config.redis, self.args['hosts'])
        target_host = self.args['host']['fqdn']

        any_host = list(self.args['hosts'].keys())[0]
        self.deploy_api.wait(
            [
                self._run_sls_host(
                    any_host,
                    SERVER_STEP_DOWN_SLS,
                    pillar={'ensure_not_master': target_host},
                    environment=self.args['hosts'][any_host]['environment'],
                ),
            ]
        )

        self._delete_host()

        self._run_operation_host_group(remaining_hosts, 'metadata', pillar={'reset_sentinel': True})
        self.mlock.unlock_cluster()


@register_executor('redis_shard_host_delete')
class RedisShardHostDelete(RedisHostDelete):
    """
    Delete Redis shard host
    """

    def run(self):
        """
        1) Remove the node from the cluster
        2) Delete the host
        """
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))
        target_host = self.args['host']['fqdn']

        any_host = list(self.args['hosts'].keys())[0]
        delete_node = [
            self._run_sls_host(
                any_host,
                CLUSTER_DELETE_NODE_SLS,
                pillar={'to_delete': target_host},
                environment=self.args['hosts'][any_host]['environment'],
            ),
        ]
        self.deploy_api.wait(delete_node)
        host_group = build_host_group(self.config.redis, self.args['hosts'])
        self._run_operation_host_group(host_group, 'metadata')
        self._delete_host()
        self.mlock.unlock_cluster()
