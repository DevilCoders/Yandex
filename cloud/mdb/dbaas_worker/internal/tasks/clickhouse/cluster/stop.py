"""
ClickHouse Stop cluster executor
"""
from ...common.cluster.stop import ClusterStopExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('clickhouse_cluster_stop')
class ClickhouseClusterStop(ClusterStopExecutor):
    """
    Stop clickhouse cluster in compute
    """

    def run(self):
        self.acquire_lock()
        ch_hosts, zk_hosts = classify_host_map(self.args['hosts'])

        ch_host_group = build_host_group(self.config.clickhouse, ch_hosts)

        self._stop_host_group(ch_host_group)

        if zk_hosts:
            zk_host_group = build_host_group(self.config.zookeeper, zk_hosts)
            self._stop_host_group(zk_host_group)
        self.release_lock()
