"""
DBaaS Internal API Redis host methods
"""

from typing import Dict
from ...core.exceptions import (
    BatchHostCreationNotImplementedError,
    BatchHostDeletionNotImplementedError,
    BatchHostModifyNotImplementedError,
    DbaasClientError,
    DbaasNotImplementedError,
    HostNotExistsError,
    PreconditionFailedError,
    NoChangesError,
    ShardNotExistsError,
)
from ...core.types import Operation
from ...health import MDBH
from ...health.health import HealthInfo, ServiceRole
from ...utils import metadb
from ...utils.cluster.get import find_shard_by_name
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.helpers import first_value
from ...utils.host import create_host, delete_host, modify_host, get_host_objects, get_hosts
from ...utils.network import validate_host_public_ip
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import HostStatus
from ...utils.validation import get_flavor_by_name, assign_public_ip_changed
from .constants import MY_CLUSTER_TYPE, DEFAULT_REPLICA_PRIORITY
from .traits import HostRole, RedisTasks, ServiceType
from .pillar import RedisPillar
from .utils import group_by_shard, validate_cluster_nodes


def get_cluster_pillar(cluster):
    return RedisPillar(cluster['value'])


def get_sharded(cluster_pillar):
    return cluster_pillar.is_cluster_enabled()


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
def create_redis_hosts(cluster, host_specs, **_):
    """
    Adds hosts to the Redis cluster.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_HOSTS_API')
    if not host_specs:
        raise DbaasClientError('No hosts to add are specified')

    if len(host_specs) > 1:
        raise BatchHostCreationNotImplementedError()

    host_spec = host_specs[0]
    cid = cluster['cid']
    shard_name = host_spec.get('shard_name')
    shards = metadb.get_shards(cid=cid)

    if len(shards) > 1 and not shard_name:
        raise PreconditionFailedError('Shard name must be specified in order to add host to sharded cluster')

    shard = find_shard_by_name(shards, shard_name) if shard_name else shards[0]

    if not shard:
        raise ShardNotExistsError(shard_name)

    hosts = get_hosts(cid)
    opts = first_value(hosts)

    resource_preset_id = opts['flavor_name']
    disk_type_id = opts['disk_type_id']
    shards_dict = group_by_shard(list(hosts.values()))
    shards_dict[shard['name']].append(host_spec)
    flavor = get_flavor_by_name(resource_preset_id)
    cluster_pillar = get_cluster_pillar(cluster)
    sharded = get_sharded(cluster_pillar)
    validate_cluster_nodes(flavor, disk_type_id, resource_preset_id, sharded, shards_dict, cluster_pillar)

    task_type = RedisTasks.host_create
    host_pillar = None
    if len(shards) > 1:
        task_type = RedisTasks.shard_host_create
    else:
        priority = host_spec.get('replica_priority', DEFAULT_REPLICA_PRIORITY)
        host_pillar = RedisPillar()
        host_pillar.set_replica_priority(priority)

    shard_id = shard['shard_id']
    return create_host(
        MY_CLUSTER_TYPE,
        cluster,
        host_spec.get('zone_id'),
        host_spec.get('subnet_id'),
        assign_public_ip=host_spec['assign_public_ip'],
        shard_id=shard_id,
        task_type=task_type,
        pillar=host_pillar,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_MODIFY)
def modify_redis_host(cluster: Dict, update_host_specs: Dict, **_) -> Operation:
    """
    Modifies Redis host.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_HOSTS_API')
    if not update_host_specs:
        raise NoChangesError()

    if len(update_host_specs) > 1:
        raise BatchHostModifyNotImplementedError()

    host_spec = update_host_specs[0]
    host_name = host_spec['host_name']
    cid = cluster['cid']
    hosts = get_hosts(cid)
    if host_name not in hosts:
        raise HostNotExistsError(host_name)

    pillar = RedisPillar(metadb.get_fqdn_pillar(host_name))
    current_priority = pillar.get_replica_priority()

    changes = False
    if 'replica_priority' in host_spec:
        shards = metadb.get_shards(cid=cid)
        if len(shards) > 1:
            raise DbaasNotImplementedError('Modifying replica priority in hosts of sharded clusters is not supported')
        priority_changed = host_spec['replica_priority'] != current_priority
        changes = changes or priority_changed
        if priority_changed:
            pillar.set_replica_priority(host_spec['replica_priority'])
            metadb.update_host_pillar(cluster['cid'], host_name, pillar)

    if 'assign_public_ip' in host_spec:
        pub_ip_changed = assign_public_ip_changed(host_name, host_spec['assign_public_ip'])
        changes = changes or pub_ip_changed
        if pub_ip_changed:
            validate_host_public_ip(host_name, host_spec['assign_public_ip'])
            metadb.update_host(host_name, cluster['cid'], assign_public_ip=host_spec['assign_public_ip'])

    if not changes:
        raise NoChangesError()

    return modify_host(
        MY_CLUSTER_TYPE,
        cluster,
        host_name,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
def delete_redis_hosts(cluster, host_names, **_):
    """
    Deletes hosts from the Redis cluster.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_HOSTS_API')
    if not host_names:
        raise DbaasClientError("No hosts to delete are specified")

    if len(host_names) > 1:
        raise BatchHostDeletionNotImplementedError()

    host_name = host_names[0]
    hosts = get_hosts(cluster['cid'])
    host = hosts.get(host_name)

    if host is None:
        raise HostNotExistsError(host_name)

    shards_dict = group_by_shard(list(hosts.values()))
    shard_hosts = shards_dict[host['shard_name']]
    task_type = RedisTasks.host_delete
    if len(shard_hosts) < len(hosts):
        task_type = RedisTasks.shard_host_delete
    shard_hosts.pop()
    resource_preset_id = host['flavor_name']
    disk_type_id = host['disk_type_id']
    flavor = get_flavor_by_name(resource_preset_id)
    cluster_pillar = get_cluster_pillar(cluster)
    sharded = get_sharded(cluster_pillar)
    validate_cluster_nodes(flavor, disk_type_id, resource_preset_id, sharded, shards_dict, cluster_pillar)

    return delete_host(MY_CLUSTER_TYPE, cluster, host_name, task_type=task_type)


@register_request_handler(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
def list_redis_hosts(cluster, **_):
    """
    Returns hosts of the Redis cluster.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_HOSTS_API')
    return list_hosts(cluster)


def list_hosts(cluster):
    hosts = get_host_objects(cluster, extra_formatter=redis_host_extra_formatter)
    cid = str(cluster['cid'])

    fqdns = [host['name'] for host in hosts]
    healthinfo = MDBH.health_by_fqdns(fqdns)

    for host in hosts:
        host.update(diagnose_host(cid, host['name'], healthinfo))
    return {'hosts': hosts}


def get_masters(cluster_obj):
    hosts = list_hosts(cluster_obj)
    hosts = hosts['hosts']
    return [host['name'] for host in hosts if host.get('role') == HostRole.master]


def redis_host_extra_formatter(host: Dict, _: Dict) -> Dict:
    """
    Extra formatter for Redis host.
    """
    pillar = RedisPillar(metadb.get_fqdn_pillar(host['fqdn']))
    return {
        'replica_priority': pillar.get_replica_priority(),
    }


_HOST_ROLE_FROM_SERVICE_ROLE = {
    ServiceRole.master: HostRole.master,
    ServiceRole.replica: HostRole.replica,
    ServiceRole.unknown: HostRole.unknown,
}


def host_role_from_service_role(role: ServiceRole) -> HostRole:
    """
    Converts host role from service role.
    """
    return _HOST_ROLE_FROM_SERVICE_ROLE.get(role, HostRole.unknown)


_SERVICE_TYPE_FROM_MDBH = {
    'redis': ServiceType.redis,
    'sentinel': ServiceType.sentinel,
    'redis_cluster': ServiceType.redis_cluster,
}


def service_type_from_mdbh(name: str) -> ServiceType:
    """
    Converts service type from mdbh.
    """
    return _SERVICE_TYPE_FROM_MDBH.get(name, ServiceType.unspecified)


def diagnose_host(cid: str, hostname: str, healthinfo: HealthInfo) -> dict:
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

            if stype in (ServiceType.redis, ServiceType.redis_cluster):
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
        'role': role,
        'health': hoststatus,
        'services': services,
    }
    if system:
        hostdict['system'] = system

    return hostdict
