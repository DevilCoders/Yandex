"""
ClickHouse metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('clickhouse_metadata_update')
class ClickHouseClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on ClickHouse cluster
    """
