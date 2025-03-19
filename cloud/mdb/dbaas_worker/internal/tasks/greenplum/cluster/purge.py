"""
Greenplum purge cluster executor
"""

from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('greenplum_cluster_purge')
class GreenplumClusterPurge(ClusterPurgeExecutor):
    """
    Purge Greenplum cluster
    """
