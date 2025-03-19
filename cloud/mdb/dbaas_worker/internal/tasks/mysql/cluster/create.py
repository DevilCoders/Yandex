"""
MySQL Create cluster executor
"""

from datetime import datetime, timezone
from typing import Any, Dict, Optional
from ....providers.my_repl_key import MyReplKeyProvider
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor


CREATE = 'mysql_cluster_create'
RESTORE = 'mysql_cluster_restore'


@register_executor(RESTORE)
@register_executor(CREATE)
class MySQLClusterCreate(ClusterCreateExecutor):
    """
    Create mysql cluster in dbm and/or compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.my_repl_key = MyReplKeyProvider(config, task, queue)

    def run(self):
        self.acquire_lock()
        task_type = self.task['task_type']
        hosts = self.args['hosts']
        host_list = sorted(list(hosts.keys()))
        master = host_list[0]
        host_group = build_host_group(self.config.mysql, hosts)

        use_backup_service = self.args.get('use_backup_service', False)
        until_binlog = None  # type: Optional[str]
        if task_type == RESTORE:
            # we need to pin restore time, so it will be the same if we would restart task
            create_ts = self.task['create_ts'].timestamp()
            until_binlog = datetime.fromtimestamp(create_ts, timezone.utc).isoformat()

        for host in host_group.hosts:
            if host == master:
                host_group.hosts[host]['deploy'] = {
                    'pillar': self.make_master_pillar(
                        task_type, self.args, until_binlog, use_backup_service=use_backup_service
                    ),
                    'title': 'master',
                }
            else:
                host_group.hosts[host]['deploy'] = {
                    'pillar': self.make_replica_pillar(task_type, self.args, until_binlog),
                    'title': 'replica',
                }

        self.my_repl_key.exists(self.task['cid'])

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

    def make_master_pillar(
        self, task_type: str, args: dict, until_binlog: Optional[str], use_backup_service: bool = False
    ) -> dict:  # pylint: disable=no-self-use
        """
        Return pillar for master
        """
        pillar = {}  # type: Dict[str, Any]
        if not use_backup_service:
            pillar['do-backup'] = True

        if task_type == RESTORE:
            pillar['target-pillar-id'] = args['target-pillar-id']
            pillar['restore-from'] = args['restore-from']
            pillar['restore-from']['until-binlog-last-modified-time'] = until_binlog
        return pillar

    def make_replica_pillar(self, task_type: str, args: dict, until_binlog: Optional[str]) -> dict:
        """
        Return pillar for replica
        """
        pillar = {}  # type: Dict[str, Any]
        pillar['replica'] = True

        if task_type == RESTORE:
            pillar['mysql-master-timeout'] = (
                self.deploy_api.get_seconds_to_deadline() // self.deploy_api.default_max_attempts
            )
            pillar['target-pillar-id'] = args['target-pillar-id']
            pillar['restore-from'] = args['restore-from']
            pillar['restore-from']['until-binlog-last-modified-time'] = until_binlog
        return pillar
