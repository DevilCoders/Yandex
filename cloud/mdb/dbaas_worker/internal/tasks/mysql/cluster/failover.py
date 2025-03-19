"""
MySQL start cluster failover executor
"""

from ....providers.mysync import MySync
from ...common.deploy import BaseDeployExecutor
from ...utils import build_host_group, register_executor


@register_executor('mysql_cluster_start_failover')
class MySQLStartClusterFailover(BaseDeployExecutor):
    """
    Start failover on MySQL cluster
    """

    REPL_LAG_CHECK = 'mysql_replication_lag'

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.mysql
        self.mysync = MySync(self.config, self.task, self.queue)

    def run(self):
        """
        Trigger failover command.
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        target_host = self.args.get('target_host')
        if target_host:
            host_group = build_host_group(self.properties, {target_host: self.args['hosts'][target_host]})
            host_group.properties.juggler_checks.append(self.REPL_LAG_CHECK)
            self._health_host_group(host_group)
            self.mysync.wait_host_active(self.args['zk_hosts'], self.task['cid'], target_host)
            self.mysync.ensure_master(self.args['zk_hosts'], self.task['cid'], target_host)
        else:
            master = self.mysync.get_master(self.args['zk_hosts'], self.task['cid'])
            self.mysync.wait_other_hosts_active(self.args['zk_hosts'], self.task['cid'], master)
            self.mysync.ensure_replica(self.args['zk_hosts'], self.task['cid'], master)
        self.mlock.unlock_cluster()
