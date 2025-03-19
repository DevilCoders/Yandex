"""
Elasticsearch metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('elasticsearch_metadata_update')
class ElasticsearchClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on Elasticsearch cluster
    """
