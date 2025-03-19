"""
MongoDB Stop cluster executor
"""
from ...common.cluster.stop import ClusterStopExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('mongodb_cluster_stop')
class MongodbClusterStop(ClusterStopExecutor):
    """
    Stop mongodb cluster in compute
    """

    def run(self):
        self.acquire_lock()
        host_groups = [
            build_host_group(getattr(self.config, host_type), hosts)
            for host_type, hosts in classify_host_map(self.args['hosts']).items()
        ]
        for host_group in host_groups:
            self._stop_host_group(host_group)
        self.release_lock()
