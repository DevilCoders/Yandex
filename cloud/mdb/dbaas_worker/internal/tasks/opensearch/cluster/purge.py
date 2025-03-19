"""
Opensearch purge cluster executor
"""

from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('opensearch_cluster_purge')
class OpensearchClusterPurge(ClusterPurgeExecutor):
    """
    Purge Opensearch cluster
    """
