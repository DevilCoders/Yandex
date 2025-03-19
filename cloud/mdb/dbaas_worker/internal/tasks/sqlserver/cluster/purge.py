"""
SQLServer purge cluster executor
"""

from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('sqlserver_cluster_purge')
class SQLServerClusterPurge(ClusterPurgeExecutor):
    """
    Purge SQLServer cluster
    """
