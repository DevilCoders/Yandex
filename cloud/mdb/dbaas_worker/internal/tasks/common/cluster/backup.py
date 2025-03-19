"""
Backup create execute
"""

from abc import ABCMeta, abstractmethod
from typing import List

from ...common.deploy import BaseDeployExecutor
from ....providers.common import Change


class CreateBackupExecutor(BaseDeployExecutor, metaclass=ABCMeta):
    """
    Create backup executor
    """

    @abstractmethod
    def _choose_backup_hosts(self) -> List[str]:
        """
        Choose host on which we will run backup
        """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        backup_hosts = self._choose_backup_hosts()
        self.logger.info('Starting backup on %r', backup_hosts)
        self.deploy_api.wait(
            [
                self._run_operation_host(
                    host, 'backup', self.args['hosts'][host]['environment'], rollback=Change.noop_rollback
                )
                for host in backup_hosts
            ]
        )
        self.mlock.unlock_cluster()
