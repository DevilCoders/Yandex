"""
Opensearch Start cluster executor
"""
from ...common.cluster.start import ClusterStartExecutor
from ...utils import register_executor
from ..utils import host_groups


@register_executor('opensearch_cluster_start')
class OpensearchClusterStart(ClusterStartExecutor):
    """
    Start opensearch cluster in compute
    """

    def run(self):
        self.acquire_lock()

        master_group, data_group = host_groups(
            self.args['hosts'], self.config.opensearch_master, self.config.opensearch_data
        )

        if master_group.hosts:
            self._start_host_group(master_group)
        self._start_host_group(data_group)

        self.release_lock()
