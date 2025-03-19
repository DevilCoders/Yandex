"""
MySQL Start cluster executor
"""
from ...common.cluster.start import ClusterStartExecutor
from ...utils import build_host_group, register_executor


@register_executor('mysql_cluster_start')
class MySQLClusterStart(ClusterStartExecutor):
    """
    Start mysql cluster in compute
    """

    def run(self):
        self.acquire_lock()
        host_group = build_host_group(self.config.mysql, self.args['hosts'])
        self._start_host_group(host_group)
        self.release_lock()
