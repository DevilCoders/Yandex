"""
SQLServer Modify cluster executor
"""

from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import register_executor
from ..utils import sqlserver_build_host_groups


@register_executor('sqlserver_cluster_modify')
class SQLServerClusterModify(ClusterModifyExecutor):
    """
    Modify sqlserver cluster
    """

    def run(self):
        """
        Run modify on sqlserver hosts
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        de_host_group, witness_host_group = sqlserver_build_host_groups(self.args['hosts'], self.config)
        for host in de_host_group.hosts:
            de_host_group.hosts[host]['service_account_id'] = self.args.get('service_account_id')

        if witness_host_group.hosts:
            self._modify_hosts(witness_host_group.properties, witness_host_group.hosts)
        self._modify_hosts(de_host_group.properties, de_host_group.hosts)
        self.mlock.unlock_cluster()
