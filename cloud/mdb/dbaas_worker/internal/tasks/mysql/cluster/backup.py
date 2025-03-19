"""
MySQL Create backup executor
"""
from typing import List

from ....providers.mysync import MySync
from ...common.cluster.backup import CreateBackupExecutor
from ...utils import register_executor
from ....utils import get_first_key


@register_executor('mysql_cluster_create_backup')
class MySQLClusterCreateBackup(CreateBackupExecutor):
    """
    Create mysql cluster backup
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.mysync = MySync(config, task, queue)

    def _choose_backup_hosts(self) -> List[str]:
        if len(self.args['hosts']) == 1:
            return [get_first_key(self.args['hosts'])]
        master = self.mysync.get_master(self.args['zk_hosts'], self.task['cid'])
        return [master]
