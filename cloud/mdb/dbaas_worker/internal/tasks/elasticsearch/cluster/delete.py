"""
ElasticSearch Delete cluster executor
"""
from ...common.cluster.delete import ClusterDeleteExecutor
from ...utils import register_executor


@register_executor('elasticsearch_cluster_delete')
class ElasticsearchClusterDelete(ClusterDeleteExecutor):
    """
    Delete ElasticSearch cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.elasticsearch_data
