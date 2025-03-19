"""
OpenSearch Delete cluster executor
"""
from ...common.cluster.delete import ClusterDeleteExecutor
from ...utils import register_executor


@register_executor('opensearch_cluster_delete')
class OpensearchClusterDelete(ClusterDeleteExecutor):
    """
    Delete OpenSearch cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.opensearch_data
