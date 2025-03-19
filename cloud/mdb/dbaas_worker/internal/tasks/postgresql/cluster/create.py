"""
PostgreSQL Create cluster executor
"""
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor

CREATE = 'postgresql_cluster_create'
RESTORE = 'postgresql_cluster_restore'


def make_master_pillar(task_type: str, args: dict, use_backup_service=False) -> dict:
    """
    Return pillar for master
    """
    if not use_backup_service:
        pillar = {'do-backup': True}
    else:
        pillar = {}
    if 'target-pillar-id' in args:
        pillar['target-pillar-id'] = args['target-pillar-id']
    if task_type == RESTORE:
        pillar['restore-from'] = args['restore-from']
    return pillar


@register_executor(CREATE)
@register_executor(RESTORE)
class PostgreSQLClusterCreate(ClusterCreateExecutor):
    """
    Create postgresql cluster in dbm and/or compute
    """

    def _make_replica_pillar(self, task_type: str) -> dict:
        """
        Return pillar for replica
        """
        pillar = {'replica': True}
        if task_type == RESTORE:
            pillar['pg-master-timeout'] = (
                self.deploy_api.get_seconds_to_deadline() // self.deploy_api.default_max_attempts
            )
        return pillar

    def run(self):
        self.acquire_lock()
        self.update_major_version('postgres')
        host_group = build_host_group(self.config.postgresql, self.args['hosts'])
        host_list = sorted(list(host_group.hosts.keys()))

        master = host_list[0]
        replicas = host_list[1:]

        use_backup_service = self.args.get('use_backup_service', False)

        master_pillar = make_master_pillar(
            task_type=self.task['task_type'],
            args=self.args,
            use_backup_service=use_backup_service,
        )
        replica_pillar = self._make_replica_pillar(task_type=self.task['task_type'])

        host_group.hosts[master]['deploy'] = {
            'pillar': master_pillar,
            'title': 'master',
        }
        for replica in replicas:
            host_group.hosts[replica]['deploy'] = {
                'pillar': replica_pillar,
                'title': 'replica',
            }

        self._create(host_group)

        if use_backup_service:
            backup = self.args['initial_backup_info'][0]
            self.backup_db.schedule_initial_backup(
                cid=self.task['cid'],
                subcid=backup['subcid'],
                shard_id=None,
                backup_id=backup['backup_id'],
                # delay the initial backup by 4 minutes
                # to avoid conflicts with initial highstate side-effects (for example, salt-minion restart)
                start_delay=60 * 4,
            )

        self.release_lock()
