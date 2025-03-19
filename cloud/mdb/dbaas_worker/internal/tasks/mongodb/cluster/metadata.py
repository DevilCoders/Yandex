"""
MongoDB metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('mongodb_metadata_update')
class MongoDBClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on MongoDB cluster
    """
