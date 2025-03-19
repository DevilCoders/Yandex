"""
PostgreSQL Delete Metadata cluster executor
"""

from ....providers.zookeeper import Zookeeper
from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import register_executor


@register_executor('postgresql_cluster_delete_metadata')
class PostgreSQLClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    """
    Delete postgresql cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.zookeeper = Zookeeper(config, task, queue)
        self.properties = config.postgresql

    def run(self):
        self._delete_hosts()

        self.zookeeper.absent(self.args['zk_hosts'], '/pgsync/{cid}'.format(cid=self.task['cid']))
