"""
API for Redis shards management.
"""

from ...core.exceptions import PreconditionFailedError, ShardExistsError, ShardNameCollisionError, ShardNotExistsError
from ...utils import metadb
from ...utils.cluster import create as clusterutil
from ...utils.cluster.delete import delete_shard_with_task
from ...utils.cluster.get import find_shard_by_name, get_shards
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.infra import get_shard_resources
from ...utils.metadata import CreateShardMetadata, DeleteShardMetadata
from ...utils.network import get_network, get_subnets
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.validation import check_quota, get_flavor_by_name, validate_host_create_resources
from .constants import MY_CLUSTER_TYPE
from .pillar import RedisPillar
from .traits import RedisOperations, RedisRoles, RedisTasks
from .utils import group_by_shard, validate_cluster_nodes


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.CREATE)
def create_redis_shard(cluster, shard_name, host_specs, **_):
    """
    Add Redis shard.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_SHARDS_API')
    # pylint: disable=too-many-locals
    cluster_pillar = RedisPillar(cluster['value'])
    sharded = cluster_pillar.is_cluster_enabled()
    if not sharded:
        raise PreconditionFailedError('Sharding must be enabled')

    shards = metadb.get_shards(cid=cluster['cid'])
    if find_shard_by_name(shards, shard_name):
        raise ShardExistsError(shard_name)

    collision = find_shard_by_name(shards, shard_name, ignore_case=True)
    if collision:
        raise ShardNameCollisionError(shard_name, collision['name'])

    old_shard = shards[0]
    resources = get_shard_resources(old_shard['shard_id'])
    resource_preset_id = resources.resource_preset_id
    disk_size = resources.disk_size
    disk_type_id = resources.disk_type_id
    flavor = get_flavor_by_name(resource_preset_id)

    hosts = metadb.get_hosts(cid=cluster['cid'])
    shards_dict = group_by_shard(hosts)
    shards_dict[shard_name] = host_specs
    validate_cluster_nodes(flavor, disk_type_id, resource_preset_id, sharded, shards_dict, cluster_pillar)

    host_count = len(host_specs)
    check_quota(flavor, host_count, disk_size, disk_type_id)

    for host in host_specs:
        validate_host_create_resources(
            cluster_type=MY_CLUSTER_TYPE,
            role=RedisRoles.redis,
            resource_preset_id=resource_preset_id,
            geo=host['zone_id'],
            disk_type_id=disk_type_id,
            disk_size=disk_size,
        )

    network = get_network(cluster['network_id'])
    subnets = get_subnets(network)

    shard, _ = clusterutil.create_shard(
        cid=cluster['cid'],
        subcid=old_shard['subcid'],
        name=shard_name,
        flavor=flavor,
        subnets=subnets,
        volume_size=disk_size,
        host_specs=host_specs,
        disk_type_id=disk_type_id,
    )

    return create_operation(
        RedisTasks.shard_create,
        cid=cluster['cid'],
        operation_type=RedisOperations.shard_create,
        metadata=CreateShardMetadata(shard_name),
        task_args={'shard_id': shard['shard_id']},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.DELETE)
def delete_redis_shard(cluster, shard_name, **_):
    """
    Delete Redis shard.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_SHARDS_API')
    cluster_pillar = RedisPillar(cluster['value'])
    if not cluster_pillar.is_cluster_enabled():
        raise PreconditionFailedError('Sharding must be enabled')

    shards = metadb.get_shards(cid=cluster['cid'])

    shard = find_shard_by_name(shards, shard_name)
    if not shard:
        raise ShardNotExistsError(shard_name)

    if len(shards) <= 3:
        raise PreconditionFailedError('There must be at least 3 shards in a cluster')

    return delete_shard_with_task(
        cluster=cluster,
        task_type=RedisTasks.shard_delete,
        operation_type=RedisOperations.shard_delete,
        metadata=DeleteShardMetadata(shard_name),
        shard_id=shard['shard_id'],
        args={
            'shard_id': shard['shard_id'],
            'shard_name': shard['name'],
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.LIST)
def list_redis_shards(cluster, **_):
    """
    Return list of Redis shards.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_SHARDS_API')
    return {'shards': get_shards(cluster)}


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.INFO)
def get_redis_shard(cluster, shard_name, **_):
    """
    Return Redis shard.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_SHARDS_API')
    shards = metadb.get_shards(cid=cluster['cid'])
    shard = find_shard_by_name(shards, shard_name)
    if not shard:
        raise ShardNotExistsError(shard_name)

    return {'name': shard['name'], 'cluster_id': cluster['cid']}
