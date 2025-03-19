"""
Update PostgreSQL cluster TLS certs executor
"""

from ....providers.pgsync import PgSync, pgsync_cluster_prefix
from ....utils import get_first_key
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor


@register_executor('postgresql_cluster_update_tls_certs')
class PostgreSQLClusterUpdateTlsCerts(ClusterCreateExecutor):
    """
    Update TLS certs in PostgreSQL cluster
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

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        host_group = build_host_group(self.config.postgresql, self.args['hosts'])
        force_tls_certs = self.args.get('force_tls_certs', False)
        self._issue_tls(host_group, True, force_tls_certs=force_tls_certs)
        self._highstate_host_group(host_group)
        # Restart services if needed
        if self.args.get('restart', False):
            self._health_host_group(host_group, '-pre-restart')
            self.pgsync.start_maintenance(
                self.args['zk_hosts'],
                pgsync_cluster_prefix(self.task['cid']),
            )

            self._run_operation_host_group(
                host_group,
                'service',
                title='post-tls-restart',
                pillar={'service-restart': True},
                order=self.get_order(),
            )

            self.pgsync.stop_maintenance(
                self.args['zk_hosts'],
                pgsync_cluster_prefix(self.task['cid']),
            )

        self.mlock.unlock_cluster()
