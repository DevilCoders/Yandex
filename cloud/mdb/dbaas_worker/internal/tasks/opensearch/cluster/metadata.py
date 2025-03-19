"""
Opensearch metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('opensearch_metadata_update')
class OpensearchClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on Opensearch cluster
    """
