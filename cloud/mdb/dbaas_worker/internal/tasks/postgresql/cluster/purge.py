"""
PostgreSQL purge cluster executor
"""

from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('postgresql_cluster_purge')
class PostgreSQLClusterDelete(ClusterPurgeExecutor):
    """
    Purge PostgreSQL cluster
    """
