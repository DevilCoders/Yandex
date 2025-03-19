"""
PostgreSQL Database modify executor
"""
from ....common.cluster.database.modify import DatabaseModifyExecutor
from ....utils import register_executor, build_host_group
from .....providers.pgsync import PgSync, pgsync_cluster_prefix
from .....providers.common import Change
from .....providers.compute import ComputeApi
from .....providers.zookeeper import Zookeeper
from ..utils import get_master_host, SyncExtensionsException


@register_executor('postgresql_database_modify')
class PostgreSQLDatabaseModify(DatabaseModifyExecutor):
    """
    Modify postgresql database
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.pgsync = PgSync(self.config, self.task, self.queue)
        self.compute = ComputeApi(self.config, self.task, self.queue)
        self.zookeeper = Zookeeper(self.config, self.task, self.queue)
        self.properties = self.config.postgresql

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        master = get_master_host(self.task['cid'], self.pgsync, self.args['hosts'], self.args['zk_hosts'])

        error_message = self.sync_extensions(master)
        if error_message:
            raise SyncExtensionsException(error_message)

        new_db_name = self.args.get('new-database-name')
        hosts_to_run = self.hosts_to_run() if new_db_name else [master]

        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'database-modify',
                    self.args['hosts'][host]['environment'],
                    pillar=self._make_pillar(),
                )
                for host in hosts_to_run
            ]
        )

        self.mlock.unlock_cluster()

    def sync_extensions(self, fqdn) -> str:
        """
        Create extensions. Return state changes result
        """

        if 'pg_cron' in self.args.get('new-extensions', ''):
            host_group = build_host_group(self.config.postgresql, self.args['hosts'])
            self.pgsync.start_maintenance(
                self.args['zk_hosts'],
                pgsync_cluster_prefix(self.task['cid']),
            )
            self._run_operation_host_group(
                host_group,
                'service',
                title='service-restart-because-pg-cron',
                rollback=Change.noop_rollback,
                pillar={'service-restart': True},
                order=self.args['hosts'],
            )
            self.pgsync.stop_maintenance(
                self.args['zk_hosts'],
                pgsync_cluster_prefix(self.task['cid']),
            )

        if 'pg_cron' in self.args.get('deleted-extensions', ''):
            host_group = build_host_group(self.config.postgresql, self.args['hosts'])
            self._run_operation_host_group(
                host_group,
                'service',
                title='service-because-pg-cron',
                rollback=Change.noop_rollback,
                order=self.args['hosts'],
            )

        shipment_id = self._run_operation_host(
            fqdn,
            'sync-extensions',
            self.args['hosts'][fqdn]['environment'],
            title='sync-extensions',
            rollback=Change.noop_rollback,
            pillar=self._make_pillar(),
        )

        self.deploy_api.wait([shipment_id])
        state = self.deploy_api.get_salt_state_result_for_shipment(shipment_id, fqdn, 'sync-extensions')
        if state is None:
            return ''
        comment = state.get('comment', '')
        return comment if 'ERROR' in comment else ''

    def _make_pillar(self):
        return {
            'target-database': self.args['target-database'],
            'new-database-name': self.args.get('new-database-name'),
            'extension-dependencies': self.args.get('extension-dependencies'),
        }
