"""
SQLServer metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('sqlserver_metadata_update')
class SQLServerClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on SQLServer cluster
    """
