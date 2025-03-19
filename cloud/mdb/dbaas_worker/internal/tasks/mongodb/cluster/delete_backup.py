"""
MongoDB Wait For Backup API-based backup to be deleted
"""
from ....providers.metadb_backup_service import MetadbBackups
from ...common.deploy import BaseDeployExecutor
from ...utils import register_executor


@register_executor('mongodb_backup_delete')
class MongodbClusterDeleteBackup(BaseDeployExecutor):
    """
    Wait for Backup API-based backup to be deleted
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.backup_db = MetadbBackups(self.config, self.task, self.queue)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        for backup_id in self.args['backup_ids']:
            self.backup_db.wait_deleted(backup_id)
        self.mlock.unlock_cluster()
