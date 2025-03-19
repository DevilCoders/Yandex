"""
MongoDB Create cluster executor
"""

from ...common.cluster.create import ClusterCreateExecutor
from ...utils import register_executor
from ..utils import TASK_CREATE_CLUSTER, TASK_RESTORE_CLUSTER, build_new_replset_hostgroup


@register_executor(TASK_CREATE_CLUSTER)
@register_executor(TASK_RESTORE_CLUSTER)
class MongoDBClusterCreate(ClusterCreateExecutor):
    """
    Create MongoDB cluster
    """

    def run(self):
        self.acquire_lock()
        host_group = build_new_replset_hostgroup(
            hosts=self.args['hosts'], config=self.config.mongod, task=self.task, task_args=self.args
        )
        self._create(host_group)

        use_backup_service = self.args.get('use_backup_service', False)

        if use_backup_service:
            for backup in self.args['initial_backup_info']:
                self.backup_db.schedule_initial_backup(
                    cid=self.task['cid'],
                    subcid=backup['subcid'],
                    shard_id=backup['shard_id'],
                    backup_id=backup['backup_id'],
                )

        self.release_lock()
