from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import register_executor


@register_executor('metastore_cluster_delete_metadata')
class MetastoreClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    pass
