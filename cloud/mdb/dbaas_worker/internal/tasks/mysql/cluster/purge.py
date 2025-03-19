"""
MySQL purge cluster executor
"""

from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('mysql_cluster_purge')
class MySQLClusterPurge(ClusterPurgeExecutor):
    """
    Purge MySQL cluster
    """
