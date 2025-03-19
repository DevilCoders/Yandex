# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB host methods
"""

from datetime import timedelta

from ...core.exceptions import (
    BatchHostCreationNotImplementedError,
    BatchHostDeletionNotImplementedError,
    DbaasClientError,
    HostNotExistsError,
    PreconditionFailedError,
    ShardNotExistsError,
    MongoDBMixedShardingInfraConfigurationError,
)
from ...health import MDBH
from ...health.health import HealthInfo, ServiceRole
from ...utils import metadb
from ...utils.cluster.get import find_shard_by_name
from ...utils.feature_flags import has_feature_flag
from ...utils.helpers import first_value
from ...utils.host import create_host, delete_host, filter_host_map, get_host_objects, get_host_role, get_hosts
from ...utils.metadata import AddClusterHostsMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import HostStatus
from ...utils.validation import check_quota, get_flavor_by_name, validate_hosts_count
from .constants import (
    MONGOCFG_HOST_TYPE,
    MONGOD_HOST_TYPE,
    MONGOS_HOST_TYPE,
    MONGOINFRA_HOST_TYPE,
    SHARDING_INFRA_HOST_TYPES,
    MY_CLUSTER_TYPE,
    MIXED_SHARDING_CONFIG_FEATURE_FLAG,
    DEPLOY_API_RETRIES_COUNT,
    GBYTES,
)
from .traits import HostRole, MongoDBRoles, MongoDBServices, ServiceType, MongoDBOperations, MongoDBTasks
from .utils import validate_infra_hosts_count, create_mongodb_sharding_infra_subcluster


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
def create_mongodb_host(cluster, host_specs, **_):
    """
    Adds MongoDB host.
    """
    if not host_specs:
        raise DbaasClientError('No hosts to add are specified')

    if len(host_specs) > 1:
        raise BatchHostCreationNotImplementedError()

    host_spec = host_specs[0]
    role = host_spec.get('type', MongoDBRoles.mongod)
    host_type = MongoDBServices[role.value]
    hosts = get_hosts(cluster['cid'])

    _create_map = {
        MONGOD_HOST_TYPE: _create_mongod_host,
        MONGOS_HOST_TYPE: _create_infra_host,
        MONGOCFG_HOST_TYPE: _create_infra_host,
        MONGOINFRA_HOST_TYPE: _create_infra_host,
    }
    return _create_map[host_type](cluster, hosts, host_spec)


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
def delete_mongodb_host(cluster, host_names, **_):
    """
    Deletes MongoDB host.
    """
    if not host_names:
        raise DbaasClientError('No hosts to delete are specified')

    if len(host_names) > 1:
        raise BatchHostDeletionNotImplementedError()

    host_name = host_names[0]
    hosts = get_hosts(cluster['cid'])
    host = hosts.get(host_name)
    if host is None:
        raise HostNotExistsError(host_name)

    role = get_host_role(host)
    host_type = MongoDBServices[role.value]
    tier_hosts = filter_host_map(hosts, role, host['shard_id'])
    validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=role.value,  # pylint: disable=no-member
        resource_preset_id=host['flavor_name'],
        disk_type_id=host['disk_type_id'],
        hosts_count=len(tier_hosts) - 1,
    )

    if host_type in SHARDING_INFRA_HOST_TYPES:
        infra_hosts_count = {ht: len(filter_host_map(hosts, MongoDBRoles[ht])) for ht in SHARDING_INFRA_HOST_TYPES}
        infra_hosts_count[host_type] -= 1
        validate_infra_hosts_count(**infra_hosts_count)

    ret = delete_host(MY_CLUSTER_TYPE, cluster, host_name, args={'hosts_role': role})

    # In case of we are deleteing last host of subcluster, we need to delete subcluster as well
    if host_type in SHARDING_INFRA_HOST_TYPES and len(tier_hosts) - 1 == 0:
        metadb.delete_subcluster(cid=cluster['cid'], subcid=host['subcid'])

    return ret


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
def list_mongodb_hosts(cluster, **_):
    """
    Returns MongoDB hosts.
    """
    hosts = get_host_objects(cluster)
    cid = str(cluster['cid'])

    fqdns = [host['name'] for host in hosts]
    healthinfo = MDBH.health_by_fqdns(fqdns)

    for host in hosts:
        host.update(diagnose_host(cid, host['name'], host['type'], healthinfo))
    return {'hosts': hosts}


def _create_infra_host(cluster, cluster_hosts, host_spec):
    role = host_spec['type']
    host_type = MongoDBServices[role.value]

    tier_hosts = filter_host_map(cluster_hosts, role)

    mixed_host_type = MONGOINFRA_HOST_TYPE
    if host_type == MONGOINFRA_HOST_TYPE:
        mixed_host_type = MONGOCFG_HOST_TYPE
    mixed_condidions = (
        host_type in [MONGOCFG_HOST_TYPE, MONGOINFRA_HOST_TYPE],
        not has_feature_flag(MIXED_SHARDING_CONFIG_FEATURE_FLAG),
        filter_host_map(cluster_hosts, MongoDBRoles[mixed_host_type]),
    )
    if all(mixed_condidions):
        raise MongoDBMixedShardingInfraConfigurationError()

    # If there is no hosts of same type, then we need to create subluster as well
    # But return create_host operation
    if not tier_hosts:
        template_hosts = None
        for h_type in SHARDING_INFRA_HOST_TYPES:
            template_hosts = filter_host_map(cluster_hosts, MongoDBRoles[h_type])
            if template_hosts:
                break

        if not template_hosts:
            raise PreconditionFailedError('Sharding must be enabled in order to add {0} host'.format(host_type))

        opts = first_value(template_hosts)
        validate_hosts_count(
            cluster_type=MY_CLUSTER_TYPE,
            role=role.value,  # pylint: disable=no-member
            resource_preset_id=opts['flavor_name'],
            disk_type_id=opts['disk_type_id'],
            hosts_count=1,
        )

        flavor = get_flavor_by_name(opts['flavor_name'])
        check_quota(flavor, 1, opts['space_limit'], opts['disk_type_id'])

        subcluster, hosts = create_mongodb_sharding_infra_subcluster(
            cluster=cluster,
            host_specs=[host_spec],
            flavor=flavor,
            subnets=None,
            disk_size=opts['space_limit'],
            disk_type_id=opts['disk_type_id'],
            host_type=host_type,
        )
        new_host = hosts[0]
        return create_operation(
            task_type=MongoDBTasks.host_create,
            operation_type=MongoDBOperations.host_create,
            metadata=AddClusterHostsMetadata(host_names=[new_host['fqdn']]),
            cid=cluster['cid'],
            time_limit=_calculate_task_time_limit(opts['space_limit']),
            task_args={
                'host': new_host['fqdn'],
                'subcid': new_host['subcid'],
                'shard_id': None,
            },
        )
    else:
        opts = first_value(tier_hosts)
        validate_hosts_count(
            cluster_type=MY_CLUSTER_TYPE,
            role=role.value,  # pylint: disable=no-member
            resource_preset_id=opts['flavor_name'],
            disk_type_id=opts['disk_type_id'],
            hosts_count=len(tier_hosts) + 1,
        )

        return create_host(
            MY_CLUSTER_TYPE,
            cluster,
            host_spec['zone_id'],
            host_spec.get('subnet_id'),
            assign_public_ip=host_spec['assign_public_ip'],
            role=role,
            time_limit=_calculate_task_time_limit(opts['space_limit']),
        )


def _create_mongod_host(cluster, cluster_hosts, host_spec):
    mongod_role = MongoDBRoles.mongod
    shard_name = host_spec.get('shard_name')
    shards = metadb.get_shards(cid=cluster['cid'], role=mongod_role)

    if len(shards) > 1:
        if not shard_name:
            raise PreconditionFailedError('Shard name must be specified in order to add host to sharded cluster')

    shard = find_shard_by_name(shards, shard_name) if shard_name else shards[0]

    if not shard:
        raise ShardNotExistsError(shard_name)

    shard_id = shard['shard_id']
    shard_hosts = filter_host_map(cluster_hosts, mongod_role, shard_id)

    opts = first_value(shard_hosts)
    validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=mongod_role.value,  # pylint: disable=no-member
        resource_preset_id=opts['flavor_name'],
        disk_type_id=opts['disk_type_id'],
        hosts_count=len(shard_hosts) + 1,
    )

    return create_host(
        MY_CLUSTER_TYPE,
        cluster,
        host_spec['zone_id'],
        host_spec.get('subnet_id'),
        assign_public_ip=host_spec['assign_public_ip'],
        role=mongod_role,
        shard_id=shard_id,
    )


_HOST_ROLE_FROM_SERVICE_ROLE = {
    ServiceRole.master: HostRole.primary,
    ServiceRole.replica: HostRole.secondary,
    ServiceRole.unknown: HostRole.unknown,
}


def host_role_from_service_role(role: ServiceRole) -> HostRole:
    """
    Converts host role from service role.
    """
    return _HOST_ROLE_FROM_SERVICE_ROLE.get(role, HostRole.unknown)


_SERVICE_TYPE_FROM_MDBH = {
    'mongod': ServiceType.mongod,
    'mongos': ServiceType.mongos,
    'mongocfg': ServiceType.mongocfg,
}


def service_type_from_mdbh(name: str) -> ServiceType:
    """
    Converts service type from mdbh.
    """
    return _SERVICE_TYPE_FROM_MDBH.get(name, ServiceType.unspecified)


def diagnose_host(cid: str, hostname: str, hosttype: MongoDBRoles, healthinfo: HealthInfo) -> dict:
    """
    Enriches hosts with their health info.
    """
    services = []
    role = HostRole.unknown
    system = {}

    hoststatus = HostStatus.unknown
    hosthealth = healthinfo.host(hostname, cid)
    if hosthealth is not None:
        hoststatus = hosthealth.status()
        for shealth in hosthealth.services():
            stype = service_type_from_mdbh(shealth.name)

            if shealth.name == hosttype.name or (hosttype.name == 'mongoinfra' and shealth.name == 'mongos'):
                role = host_role_from_service_role(shealth.role)

            services.append(
                {
                    'type': stype,
                    'health': shealth.status,
                }
            )

        system = hosthealth.system_dict()

    hostdict = {
        'name': hostname,
        'type': hosttype,
        'role': role,
        'health': hoststatus,
        'services': services,
    }
    if system:
        hostdict['system'] = system

    return hostdict


def _calculate_task_time_limit(space_limit):
    """
    Calculate time_limit for add host operation
    """
    min_time_limit_hours = 24

    # I assume, that mongo will be able to replicate at least 300 GB per 24 hours
    # and we multiply it to DEPLOY_API_RETRIES_COUNT
    time_limit_hours = max(
        min_time_limit_hours,
        ((space_limit / GBYTES) / 300) * 24 * DEPLOY_API_RETRIES_COUNT,
    )
    return timedelta(hours=time_limit_hours)
