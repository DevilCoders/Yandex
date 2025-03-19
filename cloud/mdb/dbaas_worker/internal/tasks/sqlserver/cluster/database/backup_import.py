"""
Backup import executer
"""

from .check_ext_storage_mixin import CheckExtStorageMixin
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor
from .....providers.common import Change


@register_executor('sqlserver_database_backup_import')
class DatabaseBackupImportExecutor(BaseDeployExecutor, CheckExtStorageMixin):
    """
    Backup import executer, no doubts
    """

    def _make_pillar(self):
        return {
            'target-database': self.args['target-database'],
            'backup-import': self.args['backup-import'],
            'db-restore-from': self.args['db-restore-from'],
        }

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        pillar = self._make_pillar()
        self.check_ext_storage('backup-import', pillar)
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'database-backup-import',
                    self.args['hosts'][host]['environment'],
                    pillar=pillar,
                    rollback=Change.noop_rollback,
                )
                for host in self.hosts_to_run()
            ]
        )
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'database-create',
                    self.args['hosts'][host]['environment'],
                    pillar=pillar,
                )
                for host in self.hosts_to_run()
            ]
        )
        self.mlock.unlock_cluster()

    def hosts_to_run(self):
        """
        Returns list of host on which quick operation should be executed
        """
        return self.args['hosts']
