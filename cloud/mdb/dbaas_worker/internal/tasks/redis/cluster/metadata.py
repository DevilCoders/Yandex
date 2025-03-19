"""
Redis metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('redis_metadata_update')
class RedisClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on Redis cluster
    """
