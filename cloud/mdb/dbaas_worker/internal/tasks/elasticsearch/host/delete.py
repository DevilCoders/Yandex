"""
ElasticSearch host delete executor.
"""

from ..utils import classify_host_map
from ...common.host.delete import HostDeleteExecutor
from ...utils import (
    build_host_group,
    build_host_group_from_list,
    register_executor,
)
from cloud.mdb.dbaas_worker.internal.providers.host_health import HostNotHealthyError
from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException


class HostHasUniqueDataError(UserExposedException):
    pass


@register_executor('elasticsearch_host_delete')
class ElasticsearchHostDelete(HostDeleteExecutor):
    """
    Delete ElasticSearch host in dbm or compute.
    """

    UNIQUE_DATA_CHECK = 'es_unique_data'

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.elasticsearch_data

    def run(self):
        other_hosts = self.args['hosts']
        host_to_delete = self.args['host']
        lock_hosts = list(other_hosts)
        lock_hosts.append(host_to_delete['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))

        master_hosts, data_hosts = classify_host_map(other_hosts)
        master_group = build_host_group(self.config.elasticsearch_master, master_hosts)
        data_group = build_host_group(self.config.elasticsearch_data, data_hosts)
        self._health_host_group(master_group)
        self._health_host_group(data_group)

        delete_host_group = build_host_group_from_list(self.properties, [host_to_delete])
        delete_host_group.properties.juggler_checks.append(self.UNIQUE_DATA_CHECK)
        try:
            self._health_host_group(delete_host_group, timeout=2 * 60)
        except HostNotHealthyError as nhe:
            if self.UNIQUE_DATA_CHECK in nhe.message:
                raise HostHasUniqueDataError('host has unique data for some indices and thus cannot be deleted')
            raise

        self._delete_host_group_full(delete_host_group)

        self._run_operation_host_group(master_group, 'metadata')
        self._run_operation_host_group(data_group, 'metadata')

        self.mlock.unlock_cluster()
