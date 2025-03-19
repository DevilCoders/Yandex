"""
Elasticsearch purge cluster executor
"""

from ...common.cluster.purge import ClusterPurgeExecutor
from ...utils import register_executor


@register_executor('elasticsearch_cluster_purge')
class ElasticsearchClusterPurge(ClusterPurgeExecutor):
    """
    Purge Elasticsearch cluster
    """
