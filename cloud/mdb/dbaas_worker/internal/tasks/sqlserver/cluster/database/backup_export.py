"""
Backup export executer
"""

from .check_ext_storage_mixin import CheckExtStorageMixin
from ....common.deploy import BaseDeployExecutor
from ....utils import register_executor
from .....providers.common import Change


@register_executor('sqlserver_database_backup_export')
class DatabaseBackupExportExecutor(BaseDeployExecutor, CheckExtStorageMixin):
    """
    Backup export executer, no doubts
    """

    def _make_pillar(self):
        return {
            'target-database': self.args['target-database'],
            'backup-export': self.args['backup-export'],
        }

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        pillar = self._make_pillar()
        self.check_ext_storage('backup-export', pillar)
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host,
                    'database-backup-export',
                    self.args['hosts'][host]['environment'],
                    pillar=pillar,
                    rollback=Change.noop_rollback,
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
