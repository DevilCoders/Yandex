"""
Redis upgrade cluster executor
"""
from ...common.cluster.maintenance import ClusterMaintenanceExecutor
from ...utils import build_host_group, register_executor
from ..utils import get_one_by_one_order


@register_executor('redis_cluster_upgrade')
class RedisClusterUpgrade(ClusterMaintenanceExecutor):
    """
    Perform Redis cluster upgrade
    """

    def run(self):
        """
        Run upgrade on redis hosts
        """
        hosts = self.args['hosts']
        masters = self.args.get('masters')
        self.mlock.lock_cluster(sorted(hosts))
        self.update_major_version('redis')
        host_group = build_host_group(self.config.redis, hosts)
        self._run_maintenance_task(
            host_group, lambda: get_one_by_one_order(hosts, masters), operation_title='upgrade-hs'
        )
        self.mlock.unlock_cluster()
