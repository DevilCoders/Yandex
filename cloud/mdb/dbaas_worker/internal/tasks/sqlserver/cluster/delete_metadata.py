"""
SQLServer Delete Metadata cluster executor
"""

from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import register_executor


@register_executor('sqlserver_cluster_delete_metadata')
class SQLServerClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    """
    Delete sqlserver cluster metadata
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.sqlserver

    def run(self):
        self._delete_hosts()
