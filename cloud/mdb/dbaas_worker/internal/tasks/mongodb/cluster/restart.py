"""
MongoDB Restart hosts executor
"""

from ...common.cluster.restart import ClusterHostsRestartExecutor
from ...utils import register_executor
from ..utils import HOST_TYPE_ROLE


@register_executor('mongodb_cluster_restart_hosts')
class MongoDBClusterHostsRestart(ClusterHostsRestartExecutor):
    """
    Restart MongoDB hosts
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.host_type_role = HOST_TYPE_ROLE
