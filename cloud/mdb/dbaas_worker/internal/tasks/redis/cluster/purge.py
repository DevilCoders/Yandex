"""
Redis purge cluster executor
"""

from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('redis_cluster_purge')
class RedisClusterDelete(ClusterPurgeExecutor):
    """
    Purge Redis cluster
    """
