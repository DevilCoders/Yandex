"""
Redis Delete Metadata cluster executor
"""

from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import register_executor


@register_executor('redis_cluster_delete_metadata')
class RedisClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    """
    Delete Redis cluster metadata
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.redis
