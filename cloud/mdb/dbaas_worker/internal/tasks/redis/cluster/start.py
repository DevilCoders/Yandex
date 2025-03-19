"""
Redis Start cluster executor
"""
from ...common.cluster.start import ClusterStartExecutor
from ...utils import build_host_group, register_executor


@register_executor('redis_cluster_start')
class RedisClusterStart(ClusterStartExecutor):
    """
    Start redis cluster in compute
    """

    def run(self):
        self.acquire_lock()
        host_group = build_host_group(self.config.redis, self.args['hosts'])
        self._start_host_group(host_group)
        self.release_lock()
