"""
Opensearch Stop cluster executor
"""
from ...common.cluster.stop import ClusterStopExecutor
from ...utils import register_executor
from ..utils import host_groups


@register_executor('opensearch_cluster_stop')
class OpensearchClusterStop(ClusterStopExecutor):
    """
    Stop opensearch cluster in compute
    """

    def run(self):
        self.acquire_lock()
        master_group, data_group = host_groups(
            self.args['hosts'], self.config.opensearch_master, self.config.opensearch_data
        )

        self._stop_host_group(data_group)
        if master_group.hosts:
            self._stop_host_group(master_group)

        self.release_lock()
