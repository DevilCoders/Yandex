"""
Update MySQL cluster TLS certs executor
"""

from ....providers.mysync import MySync
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor
from ....utils import get_first_key


@register_executor('mysql_cluster_update_tls_certs')
class MySQLClusterUpdateTlsCerts(ClusterCreateExecutor):
    """
    Update TLS certs in MySQL cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.mysync = MySync(self.config, self.task, self.queue)

    def get_order(self):
        """
        Dynamic order resolver with mysync
        Got fron upgrade.py, probably we want to put it in one place for both calls
        """
        if len(self.args['hosts']) == 1:
            return [get_first_key(self.args['hosts'])]
        master = self.mysync.get_master(self.args['zk_hosts'], self.task['cid'])
        return [x for x in self.args['hosts'] if x != master] + [master]

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        host_group = build_host_group(self.config.mysql, self.args['hosts'])
        force_tls_certs = self.args.get('force_tls_certs', False)
        self._issue_tls(host_group, True, force_tls_certs=force_tls_certs)
        self._highstate_host_group(host_group)
        # Restart services if needed
        if self.args.get('restart', False):
            cid, zk_hosts = self.task['cid'], self.args['zk_hosts']
            self._health_host_group(host_group, '-pre-restart')
            self.mysync.start_maintenance(zk_hosts, cid)

            self._run_operation_host_group(
                host_group,
                'service',
                title='post-tls-restart',
                pillar={'service-restart': True},
                order=self.get_order(),
            )

            self.mysync.stop_maintenance(zk_hosts, cid)

        self.mlock.unlock_cluster()
