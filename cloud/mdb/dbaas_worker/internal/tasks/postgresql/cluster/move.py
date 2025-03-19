"""
PostgreSQL Move cluster executor
"""
from ...common.cluster.move import ClusterMoveExecutor
from ...utils import register_executor


@register_executor('postgresql_cluster_move')
class PostgreSQLClusterMove(ClusterMoveExecutor):
    """
    Move postgresql cluster
    """
