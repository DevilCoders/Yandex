# -*- coding: utf-8 -*-
"""
API for ClickHouse hosts management.
"""

from datetime import timedelta
from typing import List

from ...core.exceptions import (
    BatchHostCreationNotImplementedError,
    BatchHostDeletionNotImplementedError,
    DbaasClientError,
    HostNotExistsError,
    PreconditionFailedError,
    ShardNotExistsError,
)
from ...health import MDBH
from ...health.health import HealthInfo
from ...utils import metadb, validation
from ...utils.cluster.create import cluster_get_fqdn
from ...utils.cluster.get import find_shard_by_name
from ...utils.feature_flags import ensure_no_feature_flag, has_feature_flag
from ...utils.host import create_host, delete_host, find_host, get_host_objects
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import HostStatus
from .constants import MY_CLUSTER_TYPE
from .pillar import get_pillar, get_zookeeper_subcid_and_pillar
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks, ServiceType
from .utils import ch_cores_sum, get_hosts, get_zk_hosts_task_arg, validate_zk_flavor


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
def create_host_handler(cluster, host_specs, copy_schema, **_):
    """
    Adds ClickHouse host.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_HOSTS_API')

    # pylint: disable=too-many-locals
    if not host_specs:
        raise DbaasClientError('No hosts to add are specified')

    if len(host_specs) > 1:
        raise BatchHostCreationNotImplementedError()

    host_spec = host_specs[0]
    role = host_spec.get('type', ClickhouseRoles.clickhouse)

    if role == ClickhouseRoles.clickhouse:
        return create_clickhouse_host(cluster, host_spec, copy_schema)
    return create_zookeeper_host(cluster, host_spec)


def create_clickhouse_host(cluster, host_spec, copy_schema):
    """
    Adds ClickHouse host.
    """
    geo = host_spec['zone_id']

    shard_name = host_spec.get('shard_name')
    shards = metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse)
    if len(shards) > 1 and not shard_name:
        raise PreconditionFailedError('Shard name must be specified in order to add host to sharded cluster')

    shard = find_shard_by_name(shards, shard_name)
    if not shard:
        raise ShardNotExistsError(shard_name)

    ch_hosts, zk_hosts = get_hosts(cluster['cid'])

    shard_hosts = [h for h in ch_hosts if h['shard_id'] == shard['shard_id']]

    ch_pillar = get_pillar(cluster['cid'])
    if len(shard_hosts) == 1 and not (zk_hosts or ch_pillar.zk_hosts or ch_pillar.keeper_hosts):
        if len(shard_hosts) == len(ch_hosts):
            msg = 'Cluster cannot have more than 1 host in non-HA configuration'
        else:
            msg = 'Shard cannot have more than 1 host in non-HA cluster configuration'
        raise PreconditionFailedError(msg)

    host = shard_hosts[0]
    validation.validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=ClickhouseRoles.clickhouse.value,  # pylint: disable=no-member
        resource_preset_id=host['flavor_name'],
        disk_type_id=host['disk_type_id'],
        hosts_count=len(shard_hosts) + 1,
    )

    flavor = metadb.get_flavor_by_id(host['flavor'])
    fqdn = cluster_get_fqdn(geo, flavor)

    if zk_hosts:
        zk_flavor = metadb.get_flavor_by_id(zk_hosts[0]['flavor'])
        validate_zk_flavor(ch_cores_sum(ch_hosts) + flavor['cpu_limit'], zk_flavor, ClickhouseOperations.host_create)

    return create_host(
        MY_CLUSTER_TYPE,
        cluster,
        geo,
        host_spec.get('subnet_id'),
        fqdn=fqdn,
        assign_public_ip=host_spec['assign_public_ip'],
        role=ClickhouseRoles.clickhouse,
        shard_id=shard['shard_id'],
        args={
            'service_account_id': ch_pillar.service_account_id(),
            'resetup_from_replica': copy_schema,
        },
        time_limit=timedelta(days=3, hours=3) if copy_schema else None,
    )


def create_zookeeper_host(cluster, host_spec):
    """
    Adds Zookeeper host.
    """
    _, zk_hosts = get_hosts(cluster['cid'])
    if not zk_hosts:
        msg = 'Cluster does not have Zookeeper'
        raise PreconditionFailedError(msg)

    host = zk_hosts[0]
    validation.validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=ClickhouseRoles.zookeeper.value,  # pylint: disable=no-member
        resource_preset_id=host['flavor_name'],
        disk_type_id=host['disk_type_id'],
        hosts_count=len(zk_hosts) + 1,
    )

    geo = host_spec['zone_id']
    flavor = metadb.get_flavor_by_id(host['flavor'])
    fqdn = cluster_get_fqdn(geo, flavor)

    subcid, pillar = get_zookeeper_subcid_and_pillar(cluster['cid'])
    zid_added = pillar.add_node(fqdn)
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_host(
        MY_CLUSTER_TYPE,
        cluster,
        geo,
        host_spec.get('subnet_id'),
        fqdn=fqdn,
        assign_public_ip=host_spec['assign_public_ip'],
        role=ClickhouseRoles.zookeeper,
        args={
            'zk_hosts': get_zk_hosts_task_arg(hosts=zk_hosts, cluster=cluster),
            'zid_added': zid_added,
        },
        task_type=ClickhouseTasks.zookeeper_host_create,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
def delete_host_handler(cluster, host_names, **_):
    """
    Deletes ClickHouse host.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_HOSTS_API')

    if not host_names:
        raise DbaasClientError('No hosts to delete are specified')

    if len(host_names) > 1:
        raise BatchHostDeletionNotImplementedError()

    fqdn = host_names[0]
    ch_hosts, zk_hosts = get_hosts(cluster['cid'])

    host = find_host(ch_hosts, fqdn)
    if host:
        return delete_clickhouse_host(cluster, host, ch_hosts, zk_hosts)

    host = find_host(zk_hosts, fqdn)
    if host:
        return delete_zookeeper_host(cluster, host, zk_hosts)

    raise HostNotExistsError(fqdn)


def delete_clickhouse_host(cluster, host_to_delete, ch_hosts, zk_hosts):
    """
    Deletes ClickHouse host.
    """
    shard_hosts = [h for h in ch_hosts if h['shard_id'] == host_to_delete['shard_id']]

    if len(shard_hosts) <= 1:
        if len(shard_hosts) == len(ch_hosts):
            msg = 'Last host in cluster cannot be removed'
        else:
            msg = 'Last host in shard cannot be removed'
        raise PreconditionFailedError(msg)

    if not has_feature_flag('MDB_CLICKHOUSE_DISABLE_CLUSTER_CONFIGURATION_CHECKS') and len(shard_hosts) == 2:
        if len(shard_hosts) == len(ch_hosts):
            msg = 'Cluster cannot have less than 2 ClickHouse hosts in HA configuration'
        else:
            msg = 'Shard cannot have less than 2 ClickHouse hosts in HA cluster configuration'
        raise PreconditionFailedError(msg)

    pillar = get_pillar(cluster['cid'])
    keeper_hosts = pillar.zk_hosts + list(pillar.keeper_hosts.keys())
    if pillar.embedded_keeper and host_to_delete['fqdn'] in keeper_hosts:
        raise PreconditionFailedError(
            'Cluster reconfiguration affecting hosts with ClickHouse Keeper is not currently supported.'
        )

    validation.validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=ClickhouseRoles.clickhouse.value,  # pylint: disable=no-member
        resource_preset_id=host_to_delete['flavor_name'],
        disk_type_id=host_to_delete['disk_type_id'],
        hosts_count=len(shard_hosts) - 1,
    )

    return delete_host(
        cluster_type=MY_CLUSTER_TYPE,
        cluster=cluster,
        fqdn=host_to_delete['fqdn'],
        args={
            'zk_hosts': get_zk_hosts_task_arg(hosts=zk_hosts, cluster=cluster),
        },
    )


def delete_zookeeper_host(cluster, host_to_delete, zk_hosts):
    """
    Deletes Zookeeper host.
    """
    validation.validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=ClickhouseRoles.zookeeper.value,  # pylint: disable=no-member
        resource_preset_id=host_to_delete['flavor_name'],
        disk_type_id=host_to_delete['disk_type_id'],
        hosts_count=len(zk_hosts) - 1,
    )

    subcid, pillar = get_zookeeper_subcid_and_pillar(cluster['cid'])
    zid_deleted = pillar.delete_node(host_to_delete['fqdn'])
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return delete_host(
        cluster_type=MY_CLUSTER_TYPE,
        cluster=cluster,
        fqdn=host_to_delete['fqdn'],
        args={
            # we still can use the host to be deleted in the connection string
            'zk_hosts': get_zk_hosts_task_arg(hosts=zk_hosts, cluster=cluster),
            'zid_deleted': zid_deleted,
        },
        task_type=ClickhouseTasks.zookeeper_host_delete,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
def list_clickhouse_host(cluster, **_):
    """
    Returns list of ClickHouse hosts.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_HOSTS_API')

    hosts = get_host_objects(cluster)
    cid = str(cluster['cid'])

    fqdns = [host['name'] for host in hosts]
    healthinfo = MDBH.health_by_fqdns(fqdns)

    for host in hosts:
        host.update(diagnose_host(cid, host['name'], healthinfo))
    return {'hosts': hosts}


_SERVICE_TYPE_FROM_MDBH = {
    'clickhouse': ServiceType.clickhouse,
    'zookeeper': ServiceType.zookeeper,
}


def service_type_from_mdbh(name: str) -> ServiceType:
    """
    Converts service type from mdbh.
    """
    return _SERVICE_TYPE_FROM_MDBH.get(name, ServiceType.unspecified)


def diagnose_host(cid: str, hostname: str, healthinfo: HealthInfo) -> dict:
    """
    Return hosts health info.
    """
    services: List[dict] = []
    system = {}

    hoststatus = HostStatus.unknown
    hosthealth = healthinfo.host(hostname, cid)
    if hosthealth is not None:
        hoststatus = hosthealth.status()
        services = [
            {
                'type': service_type_from_mdbh(shealth.name),
                'health': shealth.status,
            }
            for shealth in hosthealth.services()
        ]

        system = hosthealth.system_dict()

    hostdict = {
        'name': hostname,
        'health': hoststatus,
        'services': services,
    }
    if system:
        hostdict['system'] = system

    return hostdict
