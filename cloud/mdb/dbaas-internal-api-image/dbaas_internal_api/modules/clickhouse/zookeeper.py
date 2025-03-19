# -*- coding: utf-8 -*-
"""
API for ZooKeeper management.
"""

import random
from typing import List, Optional, Set

from ...core.exceptions import (
    DbaasClientError,
    NonUniqueSubnetInZone,
    NoSubnetInZone,
    PreconditionFailedError,
)
from ...core.types import Operation
from ...utils import metadb
from ...utils.cluster.create import create_subcluster
from ...utils.config import cluster_type_config, minimal_zk_resources
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import Metadata
from ...utils.network import get_network, get_subnets
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ExistedHostResources, Host, parse_resources
from ...utils.validation import check_quota, get_available_geo, get_flavor_by_name, validate_hosts
from .constants import MY_CLUSTER_TYPE, ZK_SUBCLUSTER
from .pillar import ZookeeperPillar, get_pillar
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks
from .utils import (
    ch_cores_sum,
    create_operation,
    ensure_version_supported_for_cloud_storage,
    get_hosts,
    validate_zk_flavor,
)


class AddZookeeperMetadata(Metadata):
    """
    Metadata class for add ZooKeeper operations.
    """

    def _asdict(self) -> dict:
        return {}


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.ADD_ZOOKEEPER)
def add_zookeeper_handler(cluster: dict, resources: dict = None, host_specs: List[Host] = None, **_) -> Operation:
    """
    Handler for add ZooKeeper requests.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_HOSTS_API')

    ch_hosts, zk_hosts = get_hosts(cluster['cid'])
    ch_cores = ch_cores_sum(ch_hosts)
    ch_zones = set(h['geo'] for h in ch_hosts)
    zk_zones = set(h['zone_id'] for h in host_specs) if host_specs else ch_zones

    merged_resources = get_default_zk_resources(ch_cores, zk_zones).update(parse_resources(resources or {}))

    ch_pillar = get_pillar(cluster['cid'])

    if zk_hosts or ch_pillar.zk_hosts or ch_pillar.keeper_hosts:
        raise PreconditionFailedError('ZooKeeper has been already configured for this cluster')

    if ch_pillar.cloud_storage_enabled:
        ensure_version_supported_for_cloud_storage(ch_pillar.version)

    zk_flavor = get_flavor_by_name(merged_resources.resource_preset_id)
    validate_zk_flavor(ch_cores, zk_flavor)

    zk_host_specs = ensure_zk_hosts_configured(host_specs, ch_zones)
    validate_hosts(
        zk_host_specs, merged_resources.to_requested_host_resources(), ClickhouseRoles.zookeeper.value, MY_CLUSTER_TYPE
    )

    return create_zk_subcluster_with_task(cluster, zk_host_specs, zk_flavor, merged_resources)


def get_default_zk_resources(ch_cores: int, zk_zones: Set[str]) -> ExistedHostResources:
    """
    Return ZooKeeper resources depending on the number of cores on the ClickHouse hosts,
    which are available in all of the specified `zk_zones`.
    """
    zk_min_cpu = 0
    for resources in minimal_zk_resources():
        ch_subcluster_cpu = resources['ch_subcluster_cpu']
        min_zk_cores = resources['zk_host_cpu']
        if ch_cores >= ch_subcluster_cpu:
            zk_min_cpu = min_zk_cores

    # pylint: disable=no-member
    resource_preset = metadb.get_resource_preset_by_cpu(
        cluster_type=MY_CLUSTER_TYPE,
        role=ClickhouseRoles.zookeeper.value,
        flavor_type='standard',
        min_cpu=zk_min_cpu,
        zones=zk_zones,
    )

    if not resource_preset:
        zk_config = cluster_type_config(MY_CLUSTER_TYPE)['zk']
        return ExistedHostResources(
            resource_preset_id=zk_config['flavor'],
            disk_size=zk_config['volume_size'],
            disk_type_id=zk_config['disk_type_id'],
        )

    if 'disk_size_range' in resource_preset:
        disk_size = resource_preset['disk_size_range'].lower
    else:
        disk_size = resource_preset['disk_sizes'][0]
    return ExistedHostResources(
        resource_preset_id=resource_preset['preset_id'],
        disk_size=disk_size,
        disk_type_id=resource_preset['disk_type_id'],
    )


def ensure_zk_hosts_configured(host_specs: Optional[List[Host]], preferred_geo: Set[str]) -> List[Host]:
    """
    Ensure ZK hosts configured - if not configured, then randomly chosen
    """
    zk_host_count = cluster_type_config(MY_CLUSTER_TYPE)['zk']['node_count']

    if not host_specs:
        host_specs = _choice_zk_hosts(zk_host_count, preferred_geo)
    else:
        if len(host_specs) != zk_host_count:
            raise DbaasClientError('Cluster should have {0} ZooKeeper hosts'.format(zk_host_count))

    return host_specs


def _choice_zk_hosts(host_count: int, preferred_geo: Set[str]) -> List[Host]:
    """
    Choice ZooKeeper hosts.

    * use CH hosts geo
    * get random from available if need more ZK hosts then CH hosts
    """
    rnd = random.SystemRandom()
    available_geo = get_available_geo(force_filter_decommissioning=True)
    preferred_geo_list = list(preferred_geo)
    unused_geo_list = [g for g in available_geo if g not in preferred_geo]

    zk_geo = list(preferred_geo)[:host_count]
    while len(zk_geo) < host_count:
        try:
            new_geo = rnd.choice(unused_geo_list)
            unused_geo_list.remove(new_geo)
        except IndexError:
            new_geo = rnd.choice(preferred_geo_list)
        zk_geo.append(new_geo)

    return [
        {
            'zone_id': g,
            'assign_public_ip': False,
            'type': ClickhouseRoles.zookeeper,
        }
        for g in zk_geo
    ]


def create_zk_subcluster_with_task(
    cluster: dict, host_specs: List[Host], flavor: dict, resources: ExistedHostResources
) -> Operation:
    """
    Create ZooKeeper subcluster with task.
    """
    network = get_network(cluster['network_id'])
    subnets = get_subnets(network)

    check_quota(flavor, len(host_specs), resources.disk_size, resources.disk_type_id)

    create_zk_subcluster(
        cluster=cluster,
        host_specs=host_specs,
        flavor=flavor,
        subnets=subnets,
        disk_size=resources.disk_size,
        disk_type_id=resources.disk_type_id,
    )

    return create_operation(
        task_type=ClickhouseTasks.add_zookeeper,
        operation_type=ClickhouseOperations.add_zookeeper,
        metadata=AddZookeeperMetadata(),
        cid=cluster['cid'],
    )


def create_zk_subcluster(
    cluster: dict,
    host_specs: List[dict],
    flavor: dict,
    subnets: dict,
    disk_size: Optional[int],
    disk_type_id: Optional[str],
) -> None:
    """
    Create ZooKeeper subcluster.
    """
    if disk_size is None:
        raise DbaasClientError('Disk size of ZooKeeper hosts is not specified')

    try:
        subcluster, hosts = create_subcluster(
            cluster_id=cluster['cid'],
            name=ZK_SUBCLUSTER,
            flavor=flavor,
            volume_size=disk_size,
            host_specs=host_specs,
            subnets=subnets,
            roles=[ClickhouseRoles.zookeeper],
            disk_type_id=disk_type_id,
        )
    except NonUniqueSubnetInZone as error:
        raise DbaasClientError(
            f"ZooKeeper network cannot be configured as there are multiple subnets in zone '{error.zone_id}'.",
        ) from error
    except NoSubnetInZone as error:
        raise DbaasClientError(
            f"ZooKeeper network cannot be configured as there is no subnet in zone '{error.zone_id}'."
        ) from error

    zk_pillar = ZookeeperPillar.make()
    zk_pillar.add_nodes([h['fqdn'] for h in hosts])
    zk_pillar.add_users()
    #  Use version 3.6.3 in new clusters
    zk_pillar.set_version('3.6.3-1+yandex33-fd53f2f')

    metadb.add_subcluster_pillar(cluster['cid'], subcluster['subcid'], zk_pillar)
