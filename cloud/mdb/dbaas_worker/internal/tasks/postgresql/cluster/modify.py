"""
PostgreSQL Modify cluster executor
"""

from ....providers.pgsync import PgSync, PgSyncError, pgsync_cluster_prefix
from ....utils import get_first_key
from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import register_executor


@register_executor('postgresql_cluster_modify')
class PostgreSQLClusterModify(ClusterModifyExecutor):
    """
    Modify postgresql cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.pgsync = PgSync(self.config, self.task, self.queue)

    def get_order(self, action):
        """
        Dynamic order resolver with pgsync
        """
        if len(self.args['hosts']) == 1:
            return [get_first_key(self.args['hosts'])]
        try:
            master = self.pgsync.get_master(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
        except PgSyncError:
            self.pgsync.stop_maintenance(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
            master = self.pgsync.get_master(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
            self.pgsync.start_maintenance(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))

        replicas = [x for x in self.args['hosts'] if x != master]

        if action == 'change':
            return replicas + [master]
        if action == 'deploy':
            if self.args.get('reverse_order'):
                return [master] + replicas
            return replicas + [master]

        raise RuntimeError('Unexpected action: {action}'.format(action=action))

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        self.update_major_version('postgres')
        if self.args.get('restart'):
            self.pgsync.start_maintenance(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
            self._modify_hosts(self.config.postgresql, order=self.get_order)
            self.pgsync.stop_maintenance(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
        else:
            self._modify_hosts(self.config.postgresql, order=self.get_order)
        self.mlock.unlock_cluster()
