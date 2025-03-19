# -*- coding: utf-8 -*-
"""
MongoDB restore handler.
"""
from datetime import timedelta, datetime, timezone
from typing import cast, Dict, List, Optional  # noqa

from ...core.exceptions import ParseConfigError, FeatureUnavailableError, DbaasClientError
from ...core.types import CID, Operation
from ...utils import metadb
from ...utils.backup_id import generate_backup_id
from ...utils.backups import get_backup_schedule
from ...utils.cluster.get import get_cluster_info_at_time, get_subcluster, get_shards
from ...utils.feature_flags import ensure_feature_flag, has_feature_flag
from ...utils.infra import get_resources_at_rev
from ...utils.metadata import RestoreClusterMetadata
from ...utils.network import validate_security_groups
from ...utils.operation_creator import create_operation
from ...utils.pillar import store_target_pillar
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.restore_validation import assert_can_restore_to_disk, get_min_restore_disk_size
from ...utils.time import restore_time_limit
from ...utils.types import (
    ClusterDescription,
    ClusterInfo,
    Host,
    RestoreHintWithTime,
    RestoreResourcesHint,
    ShardedBackup,
)
from ...utils.validation import get_flavor_by_name
from ...utils.version import ensure_version_allowed, Version
from .constants import (
    MONGOD_HOST_TYPE,
    MY_CLUSTER_TYPE,
    PITR_RS_FEATURE_FLAG,
    PITR_DISABLE_FEATURE_FLAG,
    DEPLOY_API_RETRIES_COUNT,
    UPGRADE_PATHS,
)
from .create import create_cluster
from .pillar import MongoDBPillar
from .traits import MongoDBOperations, MongoDBRoles, MongoDBTasks
from .types import MongodbConfigSpec, OplogTimestamp, ManagedBackupMeta
from .utils import (
    validate_options,
    validate_pitr_versions,
    get_allowed_versions_to_restore,
    assert_deprecated_config,
    get_cluster_version,
    get_cluster_version_at_time,
    backup_id_from_model,
)


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
def restore_mongodb_cluster(
    description: ClusterDescription,
    backup: ShardedBackup,
    source_cluster: ClusterInfo,
    config_spec: dict,
    host_specs: List[Host],
    network_id: str,
    recovery_target_spec: Optional[dict] = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
) -> Operation:
    """
    Creates MongoDB cluster from backup.
    """
    config_host_type = MONGOD_HOST_TYPE
    try:
        spec = MongodbConfigSpec(config_spec, version_required=False)
    except ValueError as err:
        raise ParseConfigError(err)

    # making MYPY happy
    spec.version = cast(Version, spec.version)

    ensure_version_allowed(MY_CLUSTER_TYPE, spec.version)
    if recovery_target_spec is not None:
        if has_feature_flag(PITR_DISABLE_FEATURE_FLAG):
            raise FeatureUnavailableError()
        ensure_feature_flag(PITR_RS_FEATURE_FLAG)

    backup_meta = ManagedBackupMeta.load(metadb.get_managed_backup(backup.id).metadata)
    recovery_target = (
        OplogTimestamp(**recovery_target_spec)
        if recovery_target_spec is not None
        else backup_meta.default_recovery_target
    )

    backup_meta.validate_recovery_target(recovery_target)

    source_cluster = get_cluster_info_at_time(source_cluster.cid, recovery_target.datetime)

    source_resources = get_resources_at_rev(source_cluster.cid, source_cluster.rev, role=MongoDBRoles.mongod)
    dest_resources = spec.get_resources(config_host_type)

    assert_can_restore_to_disk(source_resources, dest_resources)

    dest_flavor = get_flavor_by_name(dest_resources.resource_preset_id)

    source_pillar = MongoDBPillar.load(source_cluster.pillar)
    source_version = get_cluster_version_at_time(source_cluster.cid, recovery_target.datetime, source_pillar)

    if recovery_target_spec is not None:
        validate_pitr_versions(source_version.to_string(), spec.version.to_string())

    config = {}  # type: Dict
    if spec.has_config(config_host_type):
        config = {config_host_type: spec.get_config(config_host_type)}
    validate_options(config, spec.version, dest_flavor)

    backup_start = spec.backup_start
    if not backup_start:
        backup_start = source_cluster.backup_schedule.get('start', {})

    backup_retain_period = spec.backup_retain_period
    if not backup_retain_period:
        backup_retain_period = source_cluster.backup_schedule.get('retain_period', None)

    use_backup_service = source_cluster.backup_schedule.get('use_backup_service', None)

    backup_schedule = get_backup_schedule(
        MY_CLUSTER_TYPE,
        backup_start,
        backup_retain_period,
        use_backup_service,
    )

    cluster, bucket_name = create_cluster(
        description=description,
        host_specs=host_specs,
        resources=dest_resources,
        flavor=dest_flavor,
        network_id=network_id,
        backup_schedule=backup_schedule,
        base_pillar=_make_pillar(
            source_pillar=source_pillar,
            source_version=source_version,
            spec=spec,
            config=config,
            backup_start=backup_start,
        ),
        version=spec.version,
        deletion_protection=deletion_protection,
    )
    task_args = _restore_cluster_args(
        source_cluster_id=source_cluster.cid,
        bucket_name=bucket_name,
        backup=backup,
        backup_meta=backup_meta,
        recovery_target=recovery_target,
        target_pillar_id=store_target_pillar(
            new_cluster_id=cluster['cid'], source_cluster_pillar=source_cluster.pillar
        ),
    )
    if security_group_ids:
        sg_ids = set(security_group_ids)
        validate_security_groups(sg_ids, network_id)
        task_args['security_group_ids'] = list(sg_ids)

    if use_backup_service:
        subcid = get_subcluster(role=MongoDBRoles.mongod, cluster_id=cluster['cid'])['subcid']

        backups = []

        try:
            backups.append(
                {
                    "backup_id": generate_backup_id(),
                    "subcid": get_subcluster(role=MongoDBRoles.mongocfg, cluster_id=cluster['cid'])['subcid'],
                    "shard_id": None,
                }
            )
        except RuntimeError:
            pass

        backups += [
            {
                "backup_id": generate_backup_id(),
                "subcid": subcid,
                "shard_id": shard['shard_id'],
            }
            for shard in get_shards({'cid': cluster['cid']}, role=MongoDBRoles.mongod)
        ]

        task_args['initial_backup_info'] = backups

    task_args['use_backup_service'] = use_backup_service

    return _create_task(
        cluster=cluster,
        time_limit=compute_restore_time_limit(
            dest_flavor, max(source_resources.disk_size, cast(int, dest_resources.disk_size))
        ),
        backup_id=backup.id,
        source_cid=source_cluster.cid,
        task_args=task_args,
    )


def _make_pillar(
    source_pillar: MongoDBPillar, source_version: Version, spec: MongodbConfigSpec, config: dict, backup_start: dict
) -> MongoDBPillar:
    pillar = MongoDBPillar.make()
    pillar.pillar_databases = source_pillar.pillar_databases
    pillar.pillar_users = source_pillar.pillar_users
    pillar.update_access(source_pillar.access)
    pillar.feature_compatibility_version = source_pillar.feature_compatibility_version
    pillar.update_config(config)

    pillar.update_walg_config(source_pillar.walg)
    # In case of source cluster didn't saved oplog, do not try to restore it in new cluster
    if pillar.walg.get('mongodump_oplog', None) is False:
        pillar.update_walg_config({'mongorestore_oplogReplay': False})

    if source_version != spec.version:
        spec.version = cast(Version, spec.version)
        if source_version > spec.version:
            raise DbaasClientError('Restoring on earlier version is not allowed')

        allowed_versions = get_allowed_versions_to_restore(source_version)
        if spec.version not in allowed_versions:
            raise DbaasClientError(
                'Invalid version to restore {0}, allowed values: {1}'.format(
                    spec.version, ', '.join([v.string for v in allowed_versions])
                )
            )

        assert_deprecated_config(pillar.config, UPGRADE_PATHS[spec.version.to_string()].get('deprecated_config_values'))
        pillar.feature_compatibility_version = spec.version.to_string(with_suffix=False)

    if spec.access:
        pillar.update_access(source_pillar.access)
    if backup_start:
        pillar.update_backup_start(backup_start)

    return pillar


def _restore_cluster_args(
    bucket_name: str,
    target_pillar_id: str,
    source_cluster_id: CID,
    backup: ShardedBackup,
    backup_meta: ManagedBackupMeta,
    recovery_target: OplogTimestamp,
) -> dict:
    """
    Make mongodb_cluster_restore task args

    replay starts from backup start_time (always from first operation), it's safe (oplog operations are idempotent)
    recovery point is provided by user
    """

    return {
        'target-pillar-id': target_pillar_id,
        'restore-from': {
            'cid': source_cluster_id,
            'backup-id': backup_id_from_model(backup),
            'backup-name': backup.name,
            'pitr': {
                'backup_begin': {
                    'timestamp': backup_meta.before_ts.timestamp,
                    'inc': backup_meta.before_ts.inc,
                },
                'backup_end': {
                    'timestamp': backup_meta.after_ts.timestamp,
                    'inc': backup_meta.after_ts.inc,
                },
                'target': {
                    'timestamp': recovery_target.timestamp,
                    'inc': recovery_target.inc,
                },
            },
            's3-path': backup.s3_path,
            'tool': backup.tool,
        },
        's3_buckets': {
            'backup': bucket_name,
        },
    }


def _create_task(cluster: dict, task_args: dict, source_cid: str, backup_id: str, time_limit: timedelta) -> Operation:
    return create_operation(
        task_type=MongoDBTasks.restore,
        operation_type=MongoDBOperations.restore,
        metadata=RestoreClusterMetadata(
            source_cid=source_cid,
            backup_id=backup_id,
        ),
        cid=cluster['cid'],
        task_args=task_args,
        time_limit=time_limit,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
def restore_mongodb_cluster_hints(
    source_cluster: ClusterInfo,
    backup: ShardedBackup,
) -> RestoreHintWithTime:
    """
    Compute hints for MongoDB cluster restore
    """
    source_cluster = get_cluster_info_at_time(source_cluster.cid, backup.end_time)
    source_resources = get_resources_at_rev(source_cluster.cid, source_cluster.rev)
    cur_version = get_cluster_version(source_cluster.cid, source_cluster.pillar)
    backup_meta = ManagedBackupMeta.load(metadb.get_managed_backup(backup_id_from_model(backup)).metadata)

    return RestoreHintWithTime(
        resources=RestoreResourcesHint(
            resource_preset_id=source_resources.resource_preset_id,
            disk_size=get_min_restore_disk_size(source_resources),
            role=MongoDBRoles.mongod,
        ),
        time=datetime.fromtimestamp(backup_meta.rounded_after_ts, timezone.utc),
        version=cur_version.to_string(),
        env=source_cluster.env,
        network_id=source_cluster.network_id,
    )


def compute_restore_time_limit(flavor: dict, size_in_bytes: int) -> timedelta:
    """
    Compute restore task time-limit from flavor and backup size
    """
    return restore_time_limit(
        estimate_restore_time(
            flavor=flavor,
            size_in_bytes=size_in_bytes,
        ),
        timedelta(0),
    )


def estimate_restore_time(flavor: dict, size_in_bytes: int) -> timedelta:
    """
    Estimate mongorestore time, sets max speed 10 mb/s
    """
    fetch_bytes_per_second = min(flavor['network_limit'], flavor['io_limit'], 10 * (1024**3))
    fetch_seconds = int(size_in_bytes * 1.0 / fetch_bytes_per_second)
    return timedelta(seconds=int(fetch_seconds)) * DEPLOY_API_RETRIES_COUNT
