"""
DBaaS Internal API Redis backups
"""
from typing import List

from ...core.exceptions import DbaasClientError, ParseConfigError, PreconditionFailedError
from ...core.types import CID, Operation
from ...utils.backups import get_backup_schedule, get_backup_id
from ...utils.cluster.get import get_cluster_info_at_time
from ...utils.config import get_bucket_name
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.infra import get_resources_at_rev
from ...utils.metadata import RestoreClusterMetadata
from ...utils.network import validate_security_groups
from ...utils.operation_creator import create_operation
from ...utils.pillar import store_target_pillar
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.restore_validation import assert_can_restore_to_disk, get_min_restore_disk_size
from ...utils.time import compute_restore_time_limit
from ...utils.types import ClusterDescription, ClusterInfo, Host, RestoreHint, RestoreResourcesHint, ShardedBackup
from ...utils.validation import get_flavor_by_name
from .constants import MY_CLUSTER_TYPE, UPGRADE_PATHS
from .create import create_cluster, create_pillar

from .traits import PersistenceModes, RedisOperations, RedisRoles, RedisTasks
from .types import RedisConfigSpec
from .utils import get_cluster_version


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
def restore_redis_cluster(
    description: ClusterDescription,
    backup: ShardedBackup,
    source_cluster: ClusterInfo,
    config_spec: dict,
    host_specs: List[Host],
    network_id: str,
    security_group_ids: List[str] = None,
    tls_enabled: bool = False,
    persistence_mode: str = PersistenceModes.on.value,
    deletion_protection: bool = False,
) -> Operation:
    # pylint: disable=too-many-locals
    """
    Creates Redis cluster from backup.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_BACKUPS_API')
    try:
        spec = RedisConfigSpec(config_spec)
    except ValueError as err:
        raise ParseConfigError(err)

    source_cluster = get_cluster_info_at_time(source_cluster.cid, backup.end_time)
    source_resources = get_resources_at_rev(source_cluster.cid, source_cluster.rev)

    dest_resources = spec.get_resources()
    flavor = get_flavor_by_name(dest_resources.resource_preset_id)
    assert dest_resources.disk_size is not None
    disk_size = dest_resources.disk_size
    disk_type_id = dest_resources.disk_type_id

    assert_can_restore_to_disk(source_resources, dest_resources)

    backup_start = spec.backup_start
    if not backup_start:
        backup_start = source_cluster.backup_schedule.get('start', {})

    backup_schedule = get_backup_schedule(MY_CLUSTER_TYPE, backup_start)

    cluster, private_key = create_cluster(
        description=description,
        disk_size=disk_size,
        flavor=flavor,
        host_specs=host_specs,
        network_id=network_id,
        sharded=False,
        backup_schedule=backup_schedule,
        disk_type_id=disk_type_id,
        tls_enabled=tls_enabled,
        deletion_protection=deletion_protection,
    )

    source_cid = source_cluster.cid
    version = get_cluster_version(source_cid)
    if version != spec.version:
        if version > spec.version:
            raise DbaasClientError('Restoring on earlier version is not allowed')
        version_path = UPGRADE_PATHS.get(spec.version.to_string())
        if not version_path:
            raise PreconditionFailedError('Invalid version to restore to: {new}'.format(new=spec.version))

        if version.to_string() not in version_path['from']:
            raise PreconditionFailedError(
                'Restore from {old} to {new} is not allowed'.format(old=version, new=spec.version)
            )

    create_pillar(
        env=source_cluster.env,
        cluster=cluster,
        spec=spec,
        backup_start=backup_schedule['start'],
        private_key=private_key,
        flavor=flavor,
        sharded=False,
        tls_enabled=tls_enabled,
        persistence_mode=persistence_mode,
    )

    target_cid = cluster['cid']
    bucket_name = get_bucket_name(target_cid)
    time_limit = compute_restore_time_limit(flavor, max(source_resources.disk_size, dest_resources.disk_size))
    task_args = _restore_cluster_args(
        source_cluster_id=source_cid,
        bucket_name=bucket_name,
        backup_id=backup.id,
        backup_tool=backup.tool,
        backup_name=backup.name,
        s3_path=backup.s3_path,
        target_pillar_id=store_target_pillar(new_cluster_id=target_cid, source_cluster_pillar=source_cluster.pillar),
    )
    if security_group_ids:
        sg_ids = set(security_group_ids)
        validate_security_groups(sg_ids, network_id)
        task_args['security_group_ids'] = list(sg_ids)

    return create_operation(
        task_type=RedisTasks.restore,
        operation_type=RedisOperations.restore,
        metadata=RestoreClusterMetadata(
            source_cid=source_cid,
            backup_id=backup.id,
        ),
        cid=target_cid,
        task_args=task_args,
        time_limit=time_limit,
    )


def _restore_cluster_args(
    bucket_name: str, target_pillar_id: str, source_cluster_id: CID, backup_id: str, backup_name, s3_path, backup_tool
) -> dict:
    """
    Make redis_cluster_restore task args
    """
    return {
        'target-pillar-id': target_pillar_id,
        'restore-from': {
            'cid': source_cluster_id,
            'backup-id': get_backup_id(backup_id),
            'backup-name': backup_name,
            's3-path': s3_path,
            'tool': backup_tool,
        },
        's3_buckets': {
            'backup': bucket_name,
        },
    }


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
def restore_redis_cluster_hints(
    source_cluster: ClusterInfo,
    backup: ShardedBackup,
) -> RestoreHint:
    """
    Compute hints for Redis cluster restore
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CONSOLE_API')
    cid = source_cluster.cid
    source_cluster = get_cluster_info_at_time(cid, backup.end_time)
    source_resources = get_resources_at_rev(cid, source_cluster.rev)
    version = get_cluster_version(cid)
    return RestoreHint(
        resources=RestoreResourcesHint(
            resource_preset_id=source_resources.resource_preset_id,
            disk_size=get_min_restore_disk_size(source_resources),
            role=RedisRoles.redis,
        ),
        version=version.to_string(),
        env=source_cluster.env,
        network_id=source_cluster.network_id,
    )
