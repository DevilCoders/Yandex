"""
Opensearch Create backup executor
"""
from typing import List

from ...common.cluster.backup import CreateBackupExecutor
from ...utils import register_executor


@register_executor('opensearch_cluster_create_backup')
class OpensearchClusterCreateBackup(CreateBackupExecutor):
    """
    Create Opensearch cluster backup.
    """

    def _choose_backup_hosts(self) -> List[str]:
        hosts = self.args['hosts']
        for host, opts in hosts.items():
            return [host]
        raise RuntimeError('Unable to find cluster hosts')
