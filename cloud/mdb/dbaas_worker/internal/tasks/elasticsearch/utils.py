"""
Utilities specific to ElasticSearch clusters.
"""

from ..utils import build_host_group, get_managed_hostname
from ...utils import get_first_value
from cloud.mdb.dbaas_worker.internal.providers.host_health import HostNotHealthyError
from cloud.mdb.dbaas_worker.internal.exceptions import UserExposedException

DATA_ROLE_TYPE = 'elasticsearch_cluster.datanode'
MASTER_ROLE_TYPE = 'elasticsearch_cluster.masternode'


def classify_host_map(hosts):
    """
    Classify hosts as data or master.
    """
    master_hosts = {}
    data_hosts = {}
    for host, opts in hosts.items():
        if MASTER_ROLE_TYPE in opts.get('roles', []):
            master_hosts[host] = opts
        else:
            data_hosts[host] = opts

    return master_hosts, data_hosts


def host_groups(hosts, master_cfg, data_cfg):
    """
    Get master and data hosts groups
    """
    master_hosts, data_hosts = classify_host_map(hosts)

    master_group = build_host_group(master_cfg, master_hosts)
    data_group = build_host_group(data_cfg, data_hosts)

    data_group.properties.conductor_group_id = get_first_value(data_hosts)['subcid']
    if master_hosts:
        master_group.properties.conductor_group_id = get_first_value(master_hosts)['subcid']

    return master_group, data_group


class ClusterNotHealthyError(UserExposedException):
    pass


class ElasticsearchExecutorTrait:
    """
    ElasticsearchExecutorTrait contains helper methods useful in different tasks
    """

    def _es_cluster_health(self, master_group, data_group):
        try:
            self._health_host_group(master_group, timeout=60)
            self._health_host_group(data_group, timeout=60)
        except HostNotHealthyError as e:
            self.logger.error(f'cluster not healthy: {e!r}')
            raise ClusterNotHealthyError("cluster not healthy, some indices data not available")

    def _es_run_highstate(self, group, config):
        for host, opts in group.hosts.items():
            i_group = build_host_group(config, hosts={host: opts})
            self._highstate_host_group(i_group)
            self._health_host_group(i_group, '-post-run-highstate')

    def _es_upgrade_to_major_version(self, master_group, data_group):
        self._es_cluster_health(data_group, master_group)

        # We need to stop kibana instances so that no 2 different versions are running at the same time
        for host, opts in data_group.hosts.items():
            opts['deploy'] = {
                'pillar': {
                    'service-restart': True,
                    'stop-kibana-for-upgrade': True,
                }
            }
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_exists(managed_hostname, duration=self.task['timeout'], service='kibana-ping')
        self._es_run_highstate(data_group, self.config.elasticsearch_data)

        for opts in master_group.hosts.values():
            opts['deploy'] = {'pillar': {'service-restart': True}}
        self._es_run_highstate(master_group, self.config.elasticsearch_master)

        # Don't forget to start the kibana service
        for opts in data_group.hosts.values():
            opts['deploy']['pillar']['stop-kibana-for-upgrade'] = False
        self._run_operation_host_group(data_group, 'kibana-service')

        for host, opts in data_group.hosts.items():
            managed_hostname = get_managed_hostname(host, opts)
            self.juggler.downtime_absent(managed_hostname)
