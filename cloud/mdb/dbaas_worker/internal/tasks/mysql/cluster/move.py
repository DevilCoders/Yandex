"""
MySQL Move cluster executor
"""
from ...common.cluster.move import ClusterMoveExecutor
from ...utils import register_executor


@register_executor('mysql_cluster_move')
class MySQLClusterMove(ClusterMoveExecutor):
    """
    Move mysql cluster
    """
