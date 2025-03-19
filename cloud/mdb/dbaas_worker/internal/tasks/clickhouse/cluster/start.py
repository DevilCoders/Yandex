"""
ClickHouse Start cluster executor
"""
from ...common.cluster.start import ClusterStartExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('clickhouse_cluster_start')
class ClickhouseClusterStart(ClusterStartExecutor):
    """
    Start clickhouse cluster in compute
    """

    def run(self):
        self.acquire_lock()
        ch_hosts, zk_hosts = classify_host_map(self.args['hosts'])
        ch_subcid = next(iter(ch_hosts.values()))['subcid']

        ch_host_group = build_host_group(self.config.clickhouse, ch_hosts)
        ch_host_group.properties.conductor_group_id = ch_subcid

        if zk_hosts:
            zk_subcid = next(iter(zk_hosts.values()))['subcid']
            zk_host_group = build_host_group(self.config.zookeeper, zk_hosts)
            zk_host_group.properties.conductor_group_id = zk_subcid
            self._start_host_group(zk_host_group)

        self._start_host_group(ch_host_group)
        self.release_lock()
