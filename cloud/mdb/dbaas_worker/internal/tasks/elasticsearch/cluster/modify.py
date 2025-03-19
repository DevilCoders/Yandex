"""
Elasticsearch Modify cluster executor
"""

from ...common.cluster.modify import ClusterModifyExecutor
from ...utils import register_executor
from ..utils import host_groups, ElasticsearchExecutorTrait
from ....utils import get_first_key


@register_executor('elasticsearch_cluster_modify')
class ElasticsearchClusterModify(ClusterModifyExecutor, ElasticsearchExecutorTrait):
    """
    Modify Elasticsearch cluster
    """

    def run(self):
        """
        Run modify on elasticsearch hosts
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        hosts = self.args['hosts']
        for host in hosts:
            hosts[host]['service_account_id'] = self.args.get('service_account_id')

        master_group, data_group = host_groups(hosts, self.config.elasticsearch_master, self.config.elasticsearch_data)

        # donwload and validate extensions before apply
        host = get_first_key(data_group.hosts)
        if self.args.get('extensions'):
            self.deploy_api.wait(
                [
                    self._run_operation_host(
                        host,
                        'extensions-init',
                        data_group.hosts[host]['environment'],
                        pillar={"extensions": self.args.get("extensions", [])},
                    )
                ]
            )

        if not self.args.get('restart'):
            self._modify_hosts(data_group.properties, hosts=data_group.hosts)
            if master_group.hosts:
                self._modify_hosts(master_group.properties, hosts=master_group.hosts)
        else:
            # check health before start
            self._es_cluster_health(master_group, data_group)

            # one by one hosts updates with health check
            for host, opts in data_group.hosts.items():
                reverse_order = self.args.get('reverse_order_data', False)
                self._modify_hosts(data_group.properties, hosts={host: opts}, reverse_order=reverse_order)
            for host, opts in master_group.hosts.items():
                reverse_order = self.args.get('reverse_order_master', False)
                self._modify_hosts(master_group.properties, hosts={host: opts}, reverse_order=reverse_order)

        self.mlock.unlock_cluster()
