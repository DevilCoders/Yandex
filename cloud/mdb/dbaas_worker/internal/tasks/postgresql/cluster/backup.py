"""
PostgreSQL Create backup executor
"""
from typing import List

from ....providers.pgsync import PgSync, pgsync_cluster_prefix
from ...common.cluster.backup import CreateBackupExecutor
from ...utils import register_executor
from ....utils import get_first_key


@register_executor('postgresql_cluster_create_backup')
class PostgreSQLClusterCreateBackup(CreateBackupExecutor):
    """
    Create postgresql cluster backup
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.pgsync = PgSync(config, task, queue)

    def _choose_backup_hosts(self) -> List[str]:
        # Just return master
        # Better logic should come in - MDB-2396
        if len(self.args['hosts']) == 1:
            return [get_first_key(self.args['hosts'])]
        master = self.pgsync.get_master(self.args['zk_hosts'], pgsync_cluster_prefix(self.task['cid']))
        return [master]
