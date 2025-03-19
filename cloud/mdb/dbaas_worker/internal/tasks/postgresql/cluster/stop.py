"""
PostgreSQL Stop cluster executor
"""
from ...common.cluster.stop import ClusterStopExecutor
from ...utils import build_host_group, register_executor


@register_executor('postgresql_cluster_stop')
class PostgreSQLClusterStop(ClusterStopExecutor):
    """
    Stop postgresql cluster in compute
    """

    def run(self):
        self.acquire_lock()
        host_group = build_host_group(self.config.postgresql, self.args['hosts'])
        self._stop_host_group(host_group)
        self.release_lock()
