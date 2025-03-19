"""
PostgreSQL cluster maintenance executor
"""

from ....providers.pgsync import PgSync, pgsync_cluster_prefix
from ....utils import get_first_key
from ...common.cluster.fast_maintenance import ClusterFastMaintenanceExecutor
from ...utils import build_host_group, register_executor


@register_executor('postgresql_cluster_maintenance')
class PostgreSQLClusterMaintenance(ClusterFastMaintenanceExecutor):
    """
    Perform Maintenance on PostgreSQL cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.pgsync = PgSync(self.config, self.task, self.queue)

    def get_order(self):
        """
        Get deploy order, just like in MySQL
        """
        if len(self.args['hosts']) == 1:
            return [get_first_key(self.args['hosts'])]
        master = self.pgsync.get_master(
            self.args['zk_hosts'],
            pgsync_cluster_prefix(self.task['cid']),
        )

        return [x for x in self.args['hosts'] if x != master] + [master]

    def _run(self, task, disable_monitoring=False):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        host_group = build_host_group(self.config.postgresql, self.args['hosts'])
        if self._is_offline_maintenance():
            # Sometimes too old hosts can't start without highstate(i.e. expired cert), so omit checks here.
            if 'disable_health_check' not in self.args:
                self.args['disable_health_check'] = True
            self.args['update_tls'] = True
        self._start_offline_maintenance(host_group)
        self.pgsync.start_maintenance(
            self.args['zk_hosts'],
            pgsync_cluster_prefix(self.task['cid']),
        )

        # We can restart hosts in any order in case of offline maintenance,
        #   because we are not afraid to interrupt replication during deployment.
        #   But it will be better to disable monitoring in this case.
        if self.args.get('random_order', False) or self._is_offline_maintenance():
            task(
                host_group,
                lambda: self.args['hosts'],
                disable_monitoring=self.args.get('restart', disable_monitoring or self._is_offline_maintenance()),
            )
        else:
            task(host_group, self.get_order, disable_monitoring=self.args.get('restart', disable_monitoring))

        self.pgsync.stop_maintenance(
            self.args['zk_hosts'],
            pgsync_cluster_prefix(self.task['cid']),
        )
        self._stop_offline_maintenance(host_group)

        self.mlock.unlock_cluster()

    def run(self):
        self._run(self._run_maintenance_task)
