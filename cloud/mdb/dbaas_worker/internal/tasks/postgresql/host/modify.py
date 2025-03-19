"""
PostgreSQL Modify host executor
"""
from ...common.host.modify import HostModifyExecutor
from ...utils import register_executor
from ....providers.pgsync import PgSync, pgsync_cluster_prefix


@register_executor('postgresql_host_modify')
class PostgreSQLHostModify(HostModifyExecutor):
    """
    Modify postgresql host
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.pgsync = PgSync(config, task, queue)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        pillar = {}
        if self.args.get('restart'):
            pillar['service-restart'] = True
        if self.args.get('include-metadata'):
            pillar['include-metadata'] = True

        repl_source_change = 'replication_source' in self.args.get('pillar', {}).get('data', {}).get('pgsync', {})

        if repl_source_change or self.args.get('restart'):
            pillar.update({'include-metadata': True, 'replica': True})
            if repl_source_change:
                self.pgsync.ensure_replica(
                    self.args['host']['fqdn'], self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid'])
                )
            self.pgsync.start_maintenance(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
            self._modify_host(host=self.args['host']['fqdn'], pillar=pillar)
            self.pgsync.stop_maintenance(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))

            self._update_other_hosts_metadata(self.args['host']['fqdn'])
        else:
            self._modify_host(host=self.args['host']['fqdn'], pillar=pillar)
        self.mlock.unlock_cluster()
