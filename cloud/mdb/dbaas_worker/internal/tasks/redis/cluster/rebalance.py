"""
Redis rebalance cluster slot distribution
"""
from ...common.deploy import BaseDeployExecutor
from ...utils import build_host_group, register_executor


@register_executor('redis_cluster_rebalance')
class RedisRebalanceCluster(BaseDeployExecutor):
    """
    Rebalance slot distribution in Redis cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.redis

    def run(self):
        """
        Rebalance Redis cluster slots.
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        host_group = build_host_group(self.properties, self.args['hosts'])
        any_host = list(self.args['hosts'].keys())[0]
        self._health_host_group(host_group)
        self.deploy_api.wait(
            [
                self._run_operation_host(any_host, 'rebalance-slots', self.args['hosts'][any_host]['environment']),
            ]
        )
        self.mlock.unlock_cluster()
