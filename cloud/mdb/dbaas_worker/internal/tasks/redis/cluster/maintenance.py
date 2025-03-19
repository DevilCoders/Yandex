"""
Redis Modify cluster executor
"""
from ...common.cluster.maintenance import ClusterMaintenanceExecutor
from ...utils import build_host_group, register_executor


@register_executor('redis_cluster_maintenance')
class RedisClusterMaintenance(ClusterMaintenanceExecutor):
    """
    Perform Maintenance Redis cluster
    """

    def run(self):
        """
        Run maintenance on redis hosts
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        cluster_host_group = build_host_group(self.config.redis, self.args['hosts'])
        self._start_offline_maintenance(cluster_host_group)
        for host, opts in self.args['hosts'].items():
            host_group = build_host_group(self.config.redis, {host: opts})
            self._run_maintenance_task(host_group, get_order=lambda: [host])
        self._stop_offline_maintenance(cluster_host_group)
        self.mlock.unlock_cluster()
