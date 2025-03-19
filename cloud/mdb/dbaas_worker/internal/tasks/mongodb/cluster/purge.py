"""
MongoDB purge cluster executor
"""

from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('mongodb_cluster_purge')
class MongoDBClusterPurge(ClusterPurgeExecutor):
    """
    Purge Mongodb cluster
    """
