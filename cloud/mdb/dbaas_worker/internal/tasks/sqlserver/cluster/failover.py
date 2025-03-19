"""
SQLServer start cluster failover executor
"""

from ...common.deploy import BaseDeployExecutor
from ...utils import build_host_group, register_executor


@register_executor('sqlserver_cluster_start_failover')
class SQLServerStartClusterFailover(BaseDeployExecutor):
    """
    Start failover on SQLServer cluster
    """

    def run(self):
        """
        Trigger failover command.
        """
        target_host = self.args.get('target_host')

        self.mlock.lock_cluster(sorted(self.args['hosts']))
        # note: go-api already checked that we are not going to failover to witness-nodes
        host_group = build_host_group(self.config.sqlserver, {target_host: self.args['hosts'][target_host]})
        self._run_operation_host_group(
            host_group, 'switchover', title='switchover', pillar={'switchover': {'target_host': target_host}}
        )
        self.mlock.unlock_cluster()
