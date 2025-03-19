"""
Update Elasticsearch cluster TLS certs executor

@deprecated
Todo: remove this executor, because tls update is already supported in ./maintenance.py
"""

from ...common.cluster.create import ClusterCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('elasticsearch_cluster_update_tls_certs')
class ElasticsearchClusterUpdateTlsCerts(ClusterCreateExecutor):
    """
    Update TLS certs in Elasticsearch cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        master_hosts, data_hosts = classify_host_map(self.args['hosts'])

        master_group = build_host_group(self.config.elasticsearch_master, master_hosts)
        data_group = build_host_group(self.config.elasticsearch_data, data_hosts)
        force_tls_certs = self.args.get('force_tls_certs', False)

        self._issue_tls(master_group, True, force_tls_certs=force_tls_certs)
        self._issue_tls(data_group, True, force_tls_certs=force_tls_certs)

        self._highstate_host_group(master_group, title='update-tls')
        self._highstate_host_group(data_group, title='update-tls')

        self.mlock.unlock_cluster()
