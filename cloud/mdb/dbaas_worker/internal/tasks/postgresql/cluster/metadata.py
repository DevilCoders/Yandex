"""
PostgreSQL metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('postgresql_metadata_update')
class PostgreSQLClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on PostgreSQL cluster
    """
