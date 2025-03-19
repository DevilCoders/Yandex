"""
Greenplum metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('greenplum_metadata_update')
class GreenplumClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on Greenplum cluster
    """
