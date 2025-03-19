"""
Redis Delete cluster executor
"""

from ...common.cluster.delete import ClusterDeleteExecutor
from ...utils import register_executor


@register_executor('redis_cluster_delete')
class RedisClusterDelete(ClusterDeleteExecutor):
    """
    Delete Redis cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.redis
