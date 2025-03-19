"""
Utilities specific for Greenplum clusters.
"""
from ..utils import build_host_group
from ...utils import get_first_value
from ...providers.host_health import HostNotHealthyError
from ...exceptions import UserExposedException


MASTER_ROLE_TYPE = 'greenplum_cluster.master_subcluster'
SEGMENT_ROLE_TYPE = 'greenplum_cluster.segment_subcluster'


def classify_host_map(hosts):
    """
    Classify dict of hosts on master and segment ones.
    """
    gpdb_master_hosts = {}
    gpdb_segment_hosts = {}
    for host, opts in hosts.items():
        opts['vhost_net'] = True
        if MASTER_ROLE_TYPE in opts.get('roles', []):
            gpdb_master_hosts[host] = opts
        else:
            gpdb_segment_hosts[host] = opts

    return gpdb_master_hosts, gpdb_segment_hosts


def host_groups(hosts, master_cfg, segment_cfg):
    """
    Get master and data hosts groups
    """
    master_hosts, segment_hosts = classify_host_map(hosts)

    master_group = build_host_group(master_cfg, master_hosts)
    segment_group = build_host_group(segment_cfg, segment_hosts)

    segment_group.properties.conductor_group_id = get_first_value(segment_hosts)['subcid']
    master_group.properties.conductor_group_id = get_first_value(master_hosts)['subcid']

    return master_group, segment_group


def host_groups_part(hosts, master_cfg, segment_cfg, host):
    """
    Get master and data hosts groups with filter host
    """

    master_hosts, segment_hosts = classify_host_map(hosts)
    if host in master_hosts:
        hosts = {}
        hosts[host] = master_hosts[host]
        out_group = build_host_group(master_cfg, hosts)
        out_group.properties.conductor_group_id = get_first_value(master_hosts)['subcid']
        return out_group

    hosts = {}
    hosts[host] = segment_hosts[host]
    out_group = build_host_group(segment_cfg, hosts)
    out_group.properties.conductor_group_id = get_first_value(segment_hosts)['subcid']
    return out_group


class ClusterNotHealthyError(UserExposedException):
    pass


class GreenplumExecutorTrait:
    """
    GreenplumExecutorTrait contains helper methods useful in different tasks
    """

    def _gp_cluster_health(self, master_group, segment_group):
        try:
            self._health_host_group(master_group, timeout=60)
            self._health_host_group(segment_group, timeout=60)
        except HostNotHealthyError as e:
            self.logger.error(f'cluster not healthy: {e!r}')
            raise ClusterNotHealthyError("cluster not healthy, some indices data not available")
