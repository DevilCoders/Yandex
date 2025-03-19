"""
Greenplum Stop cluster executor
"""
from ...common.cluster.stop import ClusterStopExecutor
from ...utils import register_executor
from ..utils import host_groups


@register_executor('greenplum_cluster_stop')
class GreenplumClusterStop(ClusterStopExecutor):
    """
    Stop greenplum cluster in compute
    """

    def run(self):
        self.acquire_lock()

        master_host_group, segment_host_group = host_groups(
            self.args['hosts'], self.config.greenplum_master, self.config.greenplum_segment
        )

        self._stop_host_group(master_host_group)
        self._stop_host_group(segment_host_group)

        self.release_lock()
