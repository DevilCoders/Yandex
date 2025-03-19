"""
MongoDB Delete cluster executor
"""

from ...common.cluster.delete import ClusterDeleteExecutor
from ...utils import register_executor


@register_executor('mongodb_cluster_delete')
class MongoDBClusterDelete(ClusterDeleteExecutor):
    """
    Delete mongodb cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.mongod
