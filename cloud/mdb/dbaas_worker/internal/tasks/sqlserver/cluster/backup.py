"""
SQLServer Create backup executor
"""
from typing import List

from ...common.cluster.backup import CreateBackupExecutor
from ...utils import register_executor


@register_executor('sqlserver_cluster_create_backup')
class SQLServerClusterCreateBackup(CreateBackupExecutor):
    """
    Create sqlserver cluster backup
    """

    def _choose_backup_hosts(self) -> List[str]:
        return self.args['hosts']
