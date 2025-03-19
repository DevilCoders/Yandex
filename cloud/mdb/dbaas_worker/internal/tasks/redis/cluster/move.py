"""
Redis Move cluster executor
"""
from ...common.cluster.move import ClusterMoveExecutor
from ...utils import register_executor


@register_executor('redis_cluster_move')
class RedisClusterMove(ClusterMoveExecutor):
    """
    Move redis cluster
    """
