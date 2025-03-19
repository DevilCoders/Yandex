"""
PostgreSQL cluster failover executor
"""

from ....providers.pgsync import PgSync, PgSyncError, pgsync_cluster_prefix
from ...common.deploy import BaseDeployExecutor
from ...utils import register_executor


@register_executor('postgresql_cluster_start_failover')
class PostgreSQLStartClusterFailover(BaseDeployExecutor):
    """
    Start failover on PostgreSQL cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.postgresql
        self.pgsync = PgSync(self.config, self.task, self.queue)

    def run(self):
        """
        Trigger failover command.
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        prefix = pgsync_cluster_prefix(self.task['cid'])
        zk_hosts = self.args['zk_hosts']
        target_host = self.args.get('target_host')
        if target_host:
            self.pgsync.ensure_master(target_host, zk_hosts, prefix)
        else:
            try:
                master = self.pgsync.get_master(zk_hosts, prefix)
            except PgSyncError:
                master = None
            self.pgsync.ensure_replica(master, zk_hosts, prefix)
        self.mlock.unlock_cluster()
