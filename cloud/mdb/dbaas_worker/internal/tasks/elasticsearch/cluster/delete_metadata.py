"""
ElasticSearch Delete Metadata cluster executor
"""

from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import register_executor
from ..utils import DATA_ROLE_TYPE, MASTER_ROLE_TYPE, classify_host_map, build_host_group, get_managed_hostname


@register_executor('elasticsearch_cluster_delete_metadata')
class ElasticsearchClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    """
    Delete ElasticSearch cluster metadata
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = self.config.elasticsearch_data

    def _remove_conductor_group(self, hosts, host_type):
        for opts in hosts.values():
            if 'subcid' in opts:
                subcid = opts['subcid']
                break
        else:
            raise RuntimeError(f'Unable to find subcid for {host_type}')
        self.conductor.group_absent(subcid)

    def run(self):
        hosts = self.args['hosts']
        master_hosts, data_hosts = classify_host_map(hosts)

        self._delete_hosts()

        if master_hosts:
            self._remove_conductor_group(master_hosts, MASTER_ROLE_TYPE)
            master_host_group = build_host_group(self.config.elasticsearch_master, master_hosts)
            for host, opts in master_host_group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.eds_api.host_unregister(managed_hostname)

        self._remove_conductor_group(data_hosts, DATA_ROLE_TYPE)
        data_host_group = build_host_group(self.config.elasticsearch_data, data_hosts)
        for host, opts in data_host_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.eds_api.host_unregister(managed_hostname)
