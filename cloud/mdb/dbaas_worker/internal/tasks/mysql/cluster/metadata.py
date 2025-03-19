"""
MySQL Database create executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('mysql_metadata_update')
class MySQLClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on MySQL cluster
    """
