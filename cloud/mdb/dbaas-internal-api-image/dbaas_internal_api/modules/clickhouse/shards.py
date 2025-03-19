# -*- coding: utf-8 -*-
"""
API for ClickHouse shards management.
"""
from datetime import timedelta
from typing import Optional, Sequence

from marshmallow import Schema

from ...core.exceptions import (
    DbaasClientError,
    NoChangesError,
    PreconditionFailedError,
    ShardExistsError,
    ShardNameCollisionError,
    ShardNotExistsError,
)
from ...utils import metadb
from ...utils.cluster.create import create_shard
from ...utils.cluster.delete import delete_shard_with_task
from ...utils.cluster.get import find_shard_by_name
from ...utils.cluster.update import is_downscale, resources_diff
from ...utils.config import cluster_type_config
from ...utils.feature_flags import ensure_no_feature_flag, has_feature_flag
from ...utils.infra import get_shard_resources, get_shard_resources_strict
from ...utils.metadata import CreateShardMetadata, DeleteShardMetadata, ModifyShardMetadata
from ...utils.metadb import get_cluster_type_pillar
from ...utils.modify import update_shard_resources
from ...utils.network import get_network, get_subnets
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.renderers import render_config_set
from ...utils.types import Host
from ...utils.validation import check_quota, get_flavor_by_name, validate_host_create_resources, validate_hosts_count
from .constants import MY_CLUSTER_TYPE
from .pillar import ClickhousePillar, ClickhouseShardPillar, get_pillar, get_subcid_and_pillar
from .schemas.clusters import ClickhouseConfigSchemaV1
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks
from .utils import (
    ClickhouseShardConfigSpec,
    ch_cores_sum,
    create_operation,
    get_hosts,
    get_zk_hosts_task_arg,
    validate_zk_flavor,
)


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.CREATE)
def create_clickhouse_shard(
    cluster: dict, shard_name: str, host_specs: Sequence[dict] = None, config_spec: dict = None, copy_schema=False, **_
):
    """
    Adds ClickHouse shard.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_SHARDS_API')

    # pylint: disable=too-many-locals
    ch_hosts, zk_hosts = get_hosts(cluster['cid'])

    subcid, ch_pillar = get_subcid_and_pillar(cluster['cid'])
    shard_pillar = ClickhouseShardPillar.from_config(ch_pillar.config)

    oldest_shard = metadb.get_oldest_shard(cluster['cid'], ClickhouseRoles.clickhouse)

    spec = ClickhouseShardConfigSpec(config_spec or {})

    if spec.config:
        shard_pillar.update_config(spec.config)

    resources = get_shard_resources_strict(oldest_shard['shard_id'])
    resources.update(spec.resources)

    flavor = get_flavor_by_name(resources.resource_preset_id)
    disk_size = resources.disk_size
    disk_type_id = resources.disk_type_id

    if not host_specs:
        host_specs = [
            {
                'type': ClickhouseRoles.clickhouse,
                'zone_id': h['geo'],
                'assign_public_ip': h['assign_public_ip'],
            }
            for h in ch_hosts
            if h['shard_id'] == oldest_shard['shard_id']
        ]

    _validate_create_shard_options(
        cluster, ch_pillar, ch_hosts, zk_hosts, shard_name, host_specs, flavor, disk_size, disk_type_id
    )
    assert disk_size and disk_type_id

    network = get_network(cluster['network_id'])

    shard = create_shard(
        cid=cluster['cid'],
        name=shard_name,
        subcid=ch_hosts[0]['subcid'],
        flavor=flavor,
        subnets=get_subnets(network),
        volume_size=disk_size,
        host_specs=host_specs,
        disk_type_id=disk_type_id,
        pillar=shard_pillar,
    )[0]

    if spec.weight is not None:
        ch_pillar.update_shard(shard['shard_id'], spec.weight)
        metadb.update_subcluster_pillar(cluster['cid'], subcid, ch_pillar)

    return create_operation(
        task_type=ClickhouseTasks.shard_create,
        operation_type=ClickhouseOperations.shard_create,
        metadata=CreateShardMetadata(shard_name),
        cid=cluster['cid'],
        task_args={
            'shard_id': shard['shard_id'],
            'service_account_id': ch_pillar.service_account_id(),
            'resetup_from_replica': copy_schema,
        },
        time_limit=timedelta(days=(3 if copy_schema else 0), hours=3),
    )


def _validate_create_shard_options(
    cluster: dict,
    ch_pillar: ClickhousePillar,
    ch_hosts: Sequence[Host],
    zk_hosts: Sequence[Host],
    shard_name: str,
    host_specs: Sequence[dict],
    flavor: dict,
    disk_size: Optional[int],
    disk_type_id: Optional[str],
) -> None:
    # pylint: disable=too-many-arguments
    has_zk = zk_hosts or ch_pillar.zk_hosts or ch_pillar.keeper_hosts

    if not has_zk and len(host_specs) > 1:
        raise PreconditionFailedError('Shard cannot have more than 1 host in non-HA cluster configuration')

    validate_hosts_count(
        cluster_type=MY_CLUSTER_TYPE,
        role=ClickhouseRoles.clickhouse.value,  # pylint: disable=no-member
        resource_preset_id=flavor['name'],
        disk_type_id=disk_type_id,
        hosts_count=len(host_specs),
    )

    for host_spec in host_specs:
        if host_spec.get('type') == ClickhouseRoles.zookeeper:
            raise DbaasClientError('Shard cannot have dedicated ZooKeeper hosts')

        validate_host_create_resources(
            cluster_type=MY_CLUSTER_TYPE,
            role=ClickhouseRoles.clickhouse,
            resource_preset_id=flavor['name'],
            geo=host_spec['zone_id'],
            disk_size=disk_size,
            disk_type_id=disk_type_id,
        )

    if zk_hosts:
        ch_cores_total = ch_cores_sum(ch_hosts) + flavor['cpu_limit'] * len(host_specs)
        zk_flavor = metadb.get_flavor_by_id(zk_hosts[0]['flavor'])
        validate_zk_flavor(ch_cores_total, zk_flavor, ClickhouseOperations.shard_create)

    ch_shards = metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse)

    if not has_feature_flag('MDB_CLICKHOUSE_UNLIMITED_SHARD_COUNT'):
        shard_count_limit = cluster_type_config(MY_CLUSTER_TYPE)['shard_count_limit']
        if len(ch_shards) >= shard_count_limit:
            raise PreconditionFailedError(f'Cluster can have up to a maximum of {shard_count_limit} ClickHouse shards')

    if find_shard_by_name(ch_shards, shard_name):
        raise ShardExistsError(shard_name)

    collision = find_shard_by_name(ch_shards, shard_name, ignore_case=True)
    if collision:
        raise ShardNameCollisionError(shard_name, collision['name'])

    check_quota(flavor, len(host_specs), disk_size, disk_type_id)


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.MODIFY)
def modify_clickhouse_shard(cluster: dict, shard_name: str, config_spec: dict, _schema: Schema, **_):
    # pylint: disable=too-many-locals,too-many-arguments
    """
    Modifies ClickHouse shard.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_SHARDS_API')

    shards = metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse)
    shard = find_shard_by_name(shards, shard_name)
    if not shard:
        raise ShardNotExistsError(shard_name)

    spec = ClickhouseShardConfigSpec(config_spec)
    subcid, ch_pillar = get_subcid_and_pillar(cluster['cid'])

    changes = False
    task_args = {
        'shard_id': shard['shard_id'],
        'service_account_id': ch_pillar.service_account_id(),
        'restart': bool(_schema.context.get('restart')),
    }
    time_limit = timedelta(hours=3)

    if spec.resources:
        state, instance_type_changed = update_shard_resources(
            cid=cluster['cid'], shard_id=shard['shard_id'], cluster_type=MY_CLUSTER_TYPE, resources=spec.resources
        )
        changes = state.changes
        task_args.update(state.task_args)
        time_limit += state.time_limit

        if instance_type_changed and spec.resources.resource_preset_id:
            _validate_new_shard_flavor(cluster['cid'], shard['shard_id'], spec.resources.resource_preset_id)

    if spec.config:
        changes = True
        update_shard_config(cluster['cid'], shard, spec.config)

        if spec.config.get('geobase_uri'):
            task_args['update-geobase'] = True

    if spec.weight is not None:
        changes = True
        ch_pillar.update_shard(shard['shard_id'], spec.weight)
        metadb.update_subcluster_pillar(cluster['cid'], subcid, ch_pillar)

    if not changes:
        raise NoChangesError()

    return create_operation(
        task_type=ClickhouseTasks.shard_modify,
        operation_type=ClickhouseOperations.shard_modify,
        metadata=ModifyShardMetadata(shard_name),
        cid=cluster['cid'],
        task_args=task_args,
        time_limit=time_limit,
    )


def update_shard_config(cid: str, shard: dict, config: dict) -> None:
    """
    Update shard config in pillar.
    """
    shard_pillar = ClickhouseShardPillar(shard['value'])
    shard_pillar.update_config(config)
    metadb.update_shard_pillar(cid, shard['shard_id'], shard_pillar)


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.DELETE)
def delete_clickhouse_shard(cluster, shard_name, **_):
    """
    Deletes ClickHouse shard.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_SHARDS_API')

    ch_shards = metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse)

    ch_shard = find_shard_by_name(ch_shards, shard_name)
    if not ch_shard:
        raise ShardNotExistsError(shard_name)

    if len(ch_shards) <= 1:
        raise PreconditionFailedError('Last shard in cluster cannot be removed')

    shard_id = ch_shard['shard_id']
    subcid, pillar = get_subcid_and_pillar(cluster['cid'])
    if pillar.embedded_keeper:
        shard_hosts = metadb.get_shard_hosts(shard_id)
        keeper_hosts = pillar.zk_hosts + list(pillar.keeper_hosts.keys())
        for host in shard_hosts:
            if host['fqdn'] in keeper_hosts:
                raise PreconditionFailedError(
                    'Cluster reconfiguration affecting hosts with ClickHouse Keeper is not currently supported.'
                )

    pillar.delete_shard(shard_id)
    pillar.delete_shard_from_all_groups(shard_name)
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return delete_shard_with_task(
        cluster=cluster,
        task_type=ClickhouseTasks.shard_delete,
        operation_type=ClickhouseOperations.shard_delete,
        metadata=DeleteShardMetadata(shard_name),
        shard_id=shard_id,
        time_limit=timedelta(hours=10),
        args={
            'zk_hosts': get_zk_hosts_task_arg(cluster=cluster),
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.INFO)
def get_clickhouse_shard(cluster, shard_name, **_):
    """
    Returns ClickHouse shard.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_SHARDS_API')

    shards = metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse)
    shard = find_shard_by_name(shards, shard_name)
    if not shard:
        raise ShardNotExistsError(shard_name)

    return ShardFormatter(cluster).format(shard)


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.LIST)
def list_clickhouse_shards(cluster, **_):
    """
    Returns list of ClickHouse shards.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_SHARDS_API')

    shards = []
    formatter = ShardFormatter(cluster)
    for shard in metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse):
        shards.append(formatter.format(shard))

    return {'shards': shards}


class ShardFormatter:
    """
    Format shard objects to conform ClickhouseShardSchema.
    """

    def __init__(self, cluster: dict) -> None:
        self.cluster = cluster
        self.ch_pillar = get_pillar(cluster['cid'])
        self.default_config = get_cluster_type_pillar(MY_CLUSTER_TYPE)['data'].get('clickhouse', {}).get('config', {})

    def format(self, shard: dict) -> dict:
        """
        Return formatted shard object.
        """
        shard_id = shard['shard_id']
        resources = get_shard_resources(shard_id)

        shard_pillar = _get_shard_pillar(shard, self.ch_pillar)
        config_set = render_config_set(
            default_config=self.default_config,
            user_config=shard_pillar.config,
            schema=ClickhouseConfigSchemaV1,
            instance_type_obj=get_flavor_by_name(resources.resource_preset_id),
        )

        return {
            'name': shard['name'],
            'cluster_id': self.cluster['cid'],
            'config': {
                'clickhouse': {
                    'config': config_set.as_dict(),
                    'resources': resources,
                    'weight': self.ch_pillar.get_shard_weight(shard_id),
                },
            },
        }


def _get_shard_pillar(shard: dict, ch_pillar: ClickhousePillar) -> ClickhouseShardPillar:
    if shard['value']:
        return ClickhouseShardPillar(shard['value'])

    return ClickhouseShardPillar.from_config(ch_pillar.config)


def _validate_new_shard_flavor(cid: str, shard_id: str, resource_preset_id: str):
    """
    Check that ZooKeeper hosts have correct flavor in the target cluster configuration.
    """
    ch_hosts, zk_hosts = get_hosts(cid)
    if not zk_hosts:
        return
    new_flavor = get_flavor_by_name(resource_preset_id)
    shard_hosts = metadb.get_shard_hosts(shard_id)

    if is_downscale(resources_diff(shard_hosts, new_flavor)[0]):
        return

    ch_cores_total = ch_cores_sum(ch_hosts, shard_id) + new_flavor['cpu_limit'] * len(shard_hosts)
    zk_flavor = metadb.get_flavor_by_id(zk_hosts[0]['flavor'])
    validate_zk_flavor(ch_cores_total, zk_flavor, ClickhouseOperations.shard_modify)


@register_request_handler(MY_CLUSTER_TYPE, Resource.SHARD_GROUP, DbaasOperation.INFO)
def get_clickhouse_shard_handler(cluster, shard_group_name, **_):
    """
    Returns ClickHouse shard group.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_SHARDS_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])
    return pillar.shard_group(cluster['cid'], shard_group_name)
