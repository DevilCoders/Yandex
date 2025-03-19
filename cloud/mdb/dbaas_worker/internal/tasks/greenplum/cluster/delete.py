"""
Greenplum Delete cluster executor
"""

from ...common.cluster.delete import ClusterDeleteExecutor
from ...utils import register_executor


@register_executor('greenplum_cluster_delete')
class GreenplumClusterDelete(ClusterDeleteExecutor):
    """
    Delete greenplum cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.greenplum_master
