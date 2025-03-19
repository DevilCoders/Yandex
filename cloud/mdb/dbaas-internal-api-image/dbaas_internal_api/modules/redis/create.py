"""
DBaaS Internal API Redis cluster creation
"""

import copy
from typing import Dict, List, Optional, Tuple

from flask import current_app

from ...core.exceptions import DbaasClientError, ParseConfigError
from ...core.types import Operation
from ...utils import metadb, validation
from ...utils.backups import get_backup_schedule
from ...utils.cluster import create as clusterutil
from ...utils.config import get_bucket_name, get_environment_config
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.e2e import is_cluster_for_e2e
from ...utils.metadata import CreateClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterDescription, Host
from ...utils.version import ensure_version_allowed
from .constants import MY_CLUSTER_TYPE, DEFAULT_REPLICA_PRIORITY
from .pillar import RedisPillar
from .traits import PersistenceModes, RedisOperations, RedisRoles, RedisTasks
from .types import RedisConfigSpec
from .utils import (
    check_if_tls_available,
    group_by_shard,
    validate_disk_size,
    validate_cluster_nodes,
    validate_client_output_buffer_limit,
    validate_public_ip,
)


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
def create_cluster_handler(
    name: str,
    environment: str,
    config_spec: dict,
    host_specs: List[dict],
    network_id: str,
    description: str = None,
    labels: dict = None,
    sharded: bool = False,
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    tls_enabled: bool = False,
    deletion_protection: bool = False,
    persistence_mode: str = PersistenceModes.on.value,
    **_
) -> Operation:
    # pylint: disable=too-many-arguments, too-many-locals
    """
    Creates Redis cluster. Returns task for worker
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CLUSTER_CRUD_API')
    try:
        spec = RedisConfigSpec(config_spec)
    except ValueError as err:
        raise ParseConfigError(err)

    resources = spec.get_resources()
    disk_type_id = resources.disk_type_id
    flavor = validation.get_flavor_by_name(resources.resource_preset_id)
    disk_size = resources.disk_size
    assert disk_size is not None and disk_type_id is not None

    backup_schedule = get_backup_schedule(MY_CLUSTER_TYPE, spec.backup_start)

    cluster_description = ClusterDescription(name, environment, description, labels)
    cluster, private_key = create_cluster(
        description=cluster_description,
        disk_size=disk_size,
        flavor=flavor,
        host_specs=host_specs,
        network_id=network_id,
        sharded=sharded,
        backup_schedule=backup_schedule,
        disk_type_id=disk_type_id,
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
        tls_enabled=tls_enabled,
    )

    if spec.version is None:
        raise RuntimeError('Version is not specified in spec')

    create_pillar(
        env=environment,
        cluster=cluster,
        spec=spec,
        backup_start=backup_schedule['start'],
        private_key=private_key,
        flavor=flavor,
        sharded=sharded,
        tls_enabled=tls_enabled,
        persistence_mode=persistence_mode,
    )

    task_args = {
        's3_buckets': {
            'backup': get_bucket_name(cluster['cid']),
        },
        'sharded': sharded,
    }

    if security_group_ids is not None:
        task_args['security_group_ids'] = security_group_ids

    return create_operation(
        task_type=RedisTasks.create,
        operation_type=RedisOperations.create,
        metadata=CreateClusterMetadata(),
        cid=cluster['cid'],
        task_args=task_args,
    )


def create_cluster(
    description: ClusterDescription,
    disk_size: int,
    flavor: dict,
    host_specs: List[Host],
    network_id: str,
    sharded: bool,
    backup_schedule: dict,
    disk_type_id: Optional[str],
    maintenance_window: dict = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
    tls_enabled: bool = False,
) -> Tuple[Dict, bytes]:
    # pylint: disable=too-many-arguments, too-many-locals
    """
    Create Redis cluster, subcluster and shards in MetaDB.
    """
    validate_disk_size(flavor, disk_size)
    resource_preset_id = flavor['name']

    for node in host_specs:
        validation.validate_host_create_resources(
            cluster_type=MY_CLUSTER_TYPE,
            role=RedisRoles.redis.value,  # pylint: disable=no-member
            resource_preset_id=resource_preset_id,
            geo=node['zone_id'],
            disk_type_id=disk_type_id,
            disk_size=disk_size,
        )

    shards = group_by_shard(host_specs)
    hosts_num = len(host_specs)
    validation.check_quota(flavor, hosts_num, disk_size, disk_type_id, new_cluster=True)
    validate_cluster_nodes(flavor, disk_type_id, resource_preset_id, sharded, shards)
    validate_public_ip(host_specs, tls_enabled)

    cluster, subnets, private_key = clusterutil.create_cluster(
        cluster_type=MY_CLUSTER_TYPE,
        network_id=network_id,
        description=description,
        maintenance_window=maintenance_window,
        security_group_ids=security_group_ids,
        deletion_protection=deletion_protection,
    )
    cid = cluster['cid']

    subcluster, _ = clusterutil.create_subcluster(
        cluster_id=cid,
        name=description.name,
        roles=[RedisRoles.redis],
        disk_type_id=disk_type_id,
    )

    hosts = []
    for shard, shard_host_specs in shards.items():
        _, shard_hosts = clusterutil.create_shard(
            cid=cid,
            subcid=subcluster['subcid'],
            name=shard,
            flavor=flavor,
            subnets=subnets,
            volume_size=disk_size,
            host_specs=shard_host_specs,
            disk_type_id=disk_type_id,
        )
        hosts += shard_hosts

    if not sharded:
        for host, host_spec in zip(hosts, host_specs):
            priority = host_spec.get('replica_priority', DEFAULT_REPLICA_PRIORITY)
            host_pillar = RedisPillar()
            host_pillar.set_replica_priority(priority)
            metadb.add_host_pillar(cid, host['fqdn'], host_pillar)

    metadb.add_backup_schedule(cid, backup_schedule)

    return cluster, private_key


# pylint: disable=R0913
def create_pillar(
    env: str,
    cluster: dict,
    spec: RedisConfigSpec,
    backup_start: dict,
    private_key: bytes,
    flavor: dict,
    sharded: bool,
    tls_enabled: bool,
    persistence_mode: str,
) -> None:
    """
    Initialize Redis pillar and add it to MetaDB.
    """
    validate_client_output_buffer_limit(spec)
    access = spec.access
    config = spec.get_config()
    if 'password' not in config:
        raise DbaasClientError('Password is not specified in spec')
    version = spec.version
    ensure_version_allowed(MY_CLUSTER_TYPE, version)
    check_if_tls_available(tls_enabled, version)

    cid = cluster['cid']
    default_pillar = copy.deepcopy(current_app.config['DEFAULT_PILLAR_TEMPLATE'][MY_CLUSTER_TYPE])
    env_config = get_environment_config(MY_CLUSTER_TYPE, env)
    pillar = RedisPillar(default_pillar)
    pillar.zk_hosts = env_config['zk_hosts']
    pillar.s3_bucket = get_bucket_name(cid)
    pillar.set_cluster_private_key(private_key)
    if is_cluster_for_e2e(cluster['name']):
        pillar.set_e2e_cluster()
    pillar.set_flavor_dependent_options(flavor)
    pillar.set_cluster_mode(sharded)
    pillar.generate_renames(sharded, version)
    pillar.update_config(config)
    pillar.update_client_buffers(spec.client_output_limit_buffer_normal, spec.client_output_limit_buffer_pubsub)
    pillar.update_backup_start(backup_start)
    pillar.update_access(access)
    pillar.tls_enabled = tls_enabled
    pillar.process_persistence_mode(persistence_mode)
    metadb.add_cluster_pillar(cid, pillar)
    metadb.set_default_versions(
        cid=cid,
        subcid=None,
        shard_id=None,
        env=env,
        major_version=version,
        edition=version.edition,
        ctype=MY_CLUSTER_TYPE,
    )
