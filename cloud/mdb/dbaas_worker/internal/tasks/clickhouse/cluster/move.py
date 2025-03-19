"""
ClickHouse Move cluster executor
"""
from ...common.cluster.move import ClusterMoveExecutor
from ...utils import register_executor


@register_executor('clickhouse_cluster_move')
class ClickHouseClusterMove(ClusterMoveExecutor):
    """
    Move clickhouse cluster
    """
