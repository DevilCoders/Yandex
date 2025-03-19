"""
SQLServer Create cluster executor
"""

from ....providers.sqlserver_repl_cert import SQLServerReplCertProvider
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import register_executor
from ..utils import sqlserver_build_host_groups

CREATE = 'sqlserver_cluster_create'
RESTORE = 'sqlserver_cluster_restore'


@register_executor(RESTORE)
@register_executor(CREATE)
class SQLServerClusterCreate(ClusterCreateExecutor):
    """
    Create sqlserver cluster in compute
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.sqlserver_repl_cert = SQLServerReplCertProvider(config, task, queue)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        task_type = self.task['task_type']
        de_host_group, witness_host_group = sqlserver_build_host_groups(self.args['hosts'], self.config)
        master = sorted(list(de_host_group.hosts.keys()))[0]

        for host in de_host_group.hosts:
            if host == master:
                de_host_group.hosts[host]['deploy'] = {
                    'pillar': self.make_master_pillar(task_type, self.args),
                    'title': 'master',
                }
            else:
                de_host_group.hosts[host]['deploy'] = {
                    'pillar': self.make_replica_pillar(task_type, self.args),
                    'title': 'replica',
                }
            de_host_group.hosts[host]['service_account_id'] = self.args.get('service_account_id')

        self.sqlserver_repl_cert.exists(self.task['cid'])
        groups = [de_host_group]

        if witness_host_group.hosts:
            groups = [witness_host_group] + groups
        self._create(*groups)
        self.mlock.unlock_cluster()

    def make_master_pillar(self, task_type: str, args: dict) -> dict:  # pylint: disable=no-self-use
        """
        Return pillar for master
        """
        pillar = {'do-backup': True}
        if task_type == RESTORE:
            pillar['target-pillar-id'] = args['target-pillar-id']
            pillar['restore-from'] = args['restore-from']
        return pillar

    def make_replica_pillar(self, task_type: str, args: dict) -> dict:
        """
        Return pillar for replica
        """
        pillar = {
            'replica': True,
        }
        if task_type == RESTORE:
            pillar['sqlserver-master-timeout'] = (
                self.deploy_api.get_seconds_to_deadline() // self.deploy_api.default_max_attempts
            )
            pillar['target-pillar-id'] = args['target-pillar-id']
            pillar['restore-from'] = args['restore-from']
        return pillar
