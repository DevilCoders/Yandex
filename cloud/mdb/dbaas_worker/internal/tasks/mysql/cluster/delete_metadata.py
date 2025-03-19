"""
MySQL Delete Metadata cluster executor
"""

from ....providers.mysync import MySync
from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import register_executor


@register_executor('mysql_cluster_delete_metadata')
class MySQLClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    """
    Delete mysql cluster metadata
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.mysync = MySync(config, task, queue)
        self.properties = config.mysql

    def run(self):
        self._delete_hosts()
        self.mysync.absent(self.args['zk_hosts'], self.task['cid'])
