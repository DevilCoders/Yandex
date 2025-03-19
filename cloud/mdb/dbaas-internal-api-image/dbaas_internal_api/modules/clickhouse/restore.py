# -*- coding: utf-8 -*-
"""
DBaaS Internal API ClickHouse cluster restore
"""
from datetime import timedelta
from typing import cast, List, Optional

from ...core.exceptions import (
    DbaasClientError,
    ParseConfigError,
)
from ...core.types import CID, Operation
from ...utils import metadb
from ...utils.backups import get_backup_schedule
from ...utils.cluster.get import find_shard_by_name, get_cluster_info_at_time
from ...utils.helpers import coalesce
from ...utils.infra import get_host_resources
from ...utils.metadata import RestoreClusterMetadata
from ...utils.network import validate_security_groups
from ...utils.pillar import make_merged_pillar, store_target_pillar
from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.feature_flags import ensure_no_feature_flag, has_feature_flag
from ...utils.restore_validation import (
    assert_can_restore_to_disk,
    get_min_restore_disk_size,
    assert_cloud_storage_not_turned_off,
)
from ...utils.time import compute_restore_time_limit
from ...utils.types import (
    ClusterDescription,
    ClusterInfo,
    ExistedHostResources,
    Host,
    RequestedHostResources,
    RestoreHint,
    ShardedBackup,
    VTYPE_COMPUTE,
)
from ...utils.validation import get_flavor_by_name, validate_service_account
from ...utils.version import Version, version_validator, get_full_version, get_valid_versions
from .constants import MY_CLUSTER_TYPE, CH_PACKAGE_NAME
from .create import create_cluster
from .pillar import ClickhousePillar
from .schemas.clusters import ClickhouseConfigSchemaV1
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks
from .utils import ClickhouseConfigSpec, ClickHouseRestoreResourcesHint, create_operation


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CLUSTER, DbaasOperation.RESTORE)
def restore_clickhouse_cluster(
    description: ClusterDescription,
    backup: ShardedBackup,
    source_cluster: ClusterInfo,
    config_spec: dict,
    host_specs: List[Host],
    network_id: str,
    service_account_id: str = None,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
) -> Operation:
    """
    Creates ClickHouse cluster from backup. Returns a task for worker
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_RESTORE_CLUSTER_API')

    # pylint: disable=too-many-locals
    try:
        spec = ClickhouseConfigSpec(config_spec)
    except ValueError as exc:
        raise ParseConfigError(exc)

    backup_shard_name = _get_shard_name_from_backup(backup)
    source_cluster = get_cluster_info_at_time(cluster_id=source_cluster.cid, timestamp=backup.end_time)

    src_ch_resources = _find_source_resource(source_cluster=source_cluster, shard_name=backup_shard_name)

    dst_ch_resources, dst_zk_resources = spec.resources

    if service_account_id is not None:
        if get_flavor_by_name(dst_ch_resources.resource_preset_id)['vtype'] != VTYPE_COMPUTE:
            raise DbaasClientError('Service Accounts not supported in porto clusters')
        else:
            validate_service_account(service_account_id)

    assert_can_restore_to_disk(src_ch_resources, dst_ch_resources)

    dest_ch_flavor = get_flavor_by_name(dst_ch_resources.resource_preset_id)

    source_cluster_pillar = _get_source_pillar(source_cluster)

    assert_cloud_storage_not_turned_off(spec.cloud_storage_enabled, source_cluster_pillar.cloud_storage_enabled)

    backup_start = spec.backup_start
    if not backup_start:
        backup_start = source_cluster.backup_schedule.get('start', {})

    backup_schedule = get_backup_schedule(MY_CLUSTER_TYPE, backup_start)
    if 'use_backup_service' not in backup_schedule:
        backup_schedule['use_backup_service'] = False

    pillar = _make_pillar(
        config_spec=spec,
        backup_start=backup_schedule['start'],
        source_cluster_id=source_cluster.cid,
        source_cluster_pillar=source_cluster_pillar,
        service_account_id=service_account_id,
        shard_name=backup_shard_name,
    )

    cluster, s3_buckets = create_cluster(
        description=description,
        ch_resources=dst_ch_resources,
        zk_resources=dst_zk_resources,
        network_id=network_id,
        host_specs=host_specs,
        shard_name=backup_shard_name,
        backup_schedule=backup_schedule,
        ch_pillar=pillar,
        deletion_protection=deletion_protection,
    )

    task_args = _restore_cluster_args(
        source_cluster_id=source_cluster.cid,
        s3_buckets=s3_buckets,
        backup=backup,
        target_pillar_id=store_target_pillar(
            new_cluster_id=cluster['cid'],
            source_cluster_pillar=source_cluster_pillar.as_dict(),
        ),
        source_s3_bucket=source_cluster_pillar.s3_bucket,
        source_cloud_storage_s3_bucket=source_cluster_pillar.cloud_storage_bucket,
        service_account_id=pillar.service_account_id(),
    )
    if security_group_ids:
        sg_ids = set(security_group_ids)
        validate_security_groups(sg_ids, network_id)
        task_args['security_group_ids'] = list(sg_ids)

    if not spec.config.get('geobase_uri'):
        task_args['restore-geobase'] = True

    return _create_task(
        cluster=cluster,
        time_limit=compute_restore_time_limit(
            dest_ch_flavor, max(src_ch_resources.disk_size, cast(int, dst_ch_resources.disk_size))
        ),
        source_cid=source_cluster.cid,
        backup_id=backup.id,
        task_args=task_args,
    )


def _get_shard_name_from_backup(backup: ShardedBackup) -> str:
    if len(backup.shard_names) != 1:
        raise RuntimeError(f'Malformed backup: there can be only one shard_name in backup: {backup}')
    return backup.shard_names[0]


def _find_source_resource(source_cluster: ClusterInfo, shard_name: str) -> ExistedHostResources:
    all_source_shards = metadb.get_shards_at_rev(cid=source_cluster.cid, rev=source_cluster.rev)
    source_shard = find_shard_by_name(all_source_shards, shard_name)
    if not source_shard:
        raise RuntimeError(f'Unable to find shard {shard_name} in source shards {all_source_shards}')

    all_hosts = metadb.get_hosts_at_rev(cid=source_cluster.cid, rev=source_cluster.rev)

    for host in all_hosts:
        if host['shard_id'] == source_shard['shard_id']:
            return get_host_resources(host)

    raise RuntimeError(f'Unable to find {source_shard} resource for {source_cluster}')


def _get_source_pillar(source_cluster: ClusterInfo) -> ClickhousePillar:
    for subcluster in metadb.get_subclusters_at_rev(cid=source_cluster.cid, rev=source_cluster.rev):
        if ClickhouseRoles.clickhouse in subcluster['roles']:
            merged_dict = make_merged_pillar(source_cluster.pillar, subcluster['value'])
            return ClickhousePillar.load(merged_dict)

    raise RuntimeError(
        f'Unable to build source pillar. Looks like there are no ClickHouse subclusters in {source_cluster}'
    )


def _is_ha_cause_has_zk(zk_resources: RequestedHostResources) -> bool:
    return bool(zk_resources)


def _is_ha_case_lots_of_host(host_specs: List[Host]) -> bool:
    return len(host_specs) > 1


def options_which_affect_ddl(db_options: dict) -> dict:
    """
    Return options which affect DDL
    """
    affect_ddl = {
        (field.attribute if field.attribute else name)
        for (name, field) in ClickhouseConfigSchemaV1().fields.items()
        if field.metadata.get('affect_ddl')
    }
    return dict((k, v) for (k, v) in db_options.items() if k in affect_ddl)


def _make_pillar(
    config_spec: ClickhouseConfigSpec,
    backup_start: dict,
    source_cluster_id: CID,
    source_cluster_pillar: ClickhousePillar,
    service_account_id: Optional[str],
    shard_name: str,
) -> ClickhousePillar:
    """
    Make pillar from source_cluster pillar and db_options
    """
    ch_pillar = ClickhousePillar.make()
    ch_pillar.cluster_name = source_cluster_pillar.cluster_name or source_cluster_id
    ch_pillar.update_config(options_which_affect_ddl(source_cluster_pillar.config))
    ch_pillar.update_config(config_spec.config)
    ch_pillar.update_backup_start(source_cluster_pillar.backup_start)
    if service_account_id or source_cluster_pillar.service_account_id():
        ch_pillar.update_service_account_id(service_account_id or source_cluster_pillar.service_account_id())
    if backup_start:
        ch_pillar.update_backup_start(backup_start)
    ch_pillar.update_access(source_cluster_pillar.access)
    if config_spec.access:
        ch_pillar.update_access(config_spec.access)

    enable_testing_versions = has_feature_flag('MDB_CLICKHOUSE_TESTING_VERSIONS')
    ch_pillar.set_testing_repos(enable_testing_versions)
    if config_spec.version:
        if source_cluster_pillar.version > config_spec.version:
            raise DbaasClientError('Restoring on earlier version is deprecated')
        if enable_testing_versions:
            try:
                get_full_version(MY_CLUSTER_TYPE, config_spec.version)
            except RuntimeError:
                version_validator().ensure_version_exists(CH_PACKAGE_NAME, config_spec.version)

    ch_pillar.set_version(
        config_spec.version or max(source_cluster_pillar.version, min(get_valid_versions(MY_CLUSTER_TYPE)))
    )
    ch_pillar.add_zk_users()
    ch_pillar.add_system_users()
    ch_pillar.set_user_management_v2(ch_pillar.version >= Version(20, 6))
    ch_pillar.set_cloud_storage_enabled(
        coalesce(config_spec.cloud_storage_enabled, source_cluster_pillar.cloud_storage_enabled)
    )

    ch_pillar.set_cloud_storage_data_cache_enabled(
        coalesce(config_spec.cloud_storage_data_cache_enabled, source_cluster_pillar.cloud_storage_data_cache_enabled)
    )
    ch_pillar.set_cloud_storage_data_cache_max_size(
        coalesce(config_spec.cloud_storage_data_cache_max_size, source_cluster_pillar.cloud_storage_data_cache_max_size)
    )
    ch_pillar.set_cloud_storage_move_factor(
        coalesce(config_spec.cloud_storage_move_factor, source_cluster_pillar.cloud_storage_move_factor)
    )

    ch_pillar.update_embedded_keeper(coalesce(config_spec.embedded_keeper, source_cluster_pillar.embedded_keeper))
    ch_pillar.update_mysql_protocol(coalesce(config_spec.mysql_protocol, source_cluster_pillar.mysql_protocol))
    ch_pillar.update_postgresql_protocol(
        coalesce(config_spec.postgresql_protocol, source_cluster_pillar.postgresql_protocol)
    )

    if source_cluster_pillar.sql_user_management is True and config_spec.sql_user_management is False:
        raise DbaasClientError('SQL user management cannot be disabled after it has been enabled.')
    ch_pillar.update_sql_user_management(
        coalesce(config_spec.sql_user_management, source_cluster_pillar.sql_user_management)
    )
    if ch_pillar.sql_user_management:
        if ch_pillar.version < Version(20, 6):
            raise DbaasClientError('SQL user management is not supported in versions lower than 20.6.')
    else:
        ch_pillar.pillar_users = source_cluster_pillar.pillar_users

    ch_pillar.update_admin_password(config_spec.admin_password or source_cluster_pillar.admin_password)

    if source_cluster_pillar.sql_database_management is True and config_spec.sql_database_management is False:
        raise DbaasClientError('SQL database management cannot be disabled after it has been enabled.')
    ch_pillar.update_sql_database_management(
        coalesce(config_spec.sql_database_management, source_cluster_pillar.sql_database_management)
    )
    if ch_pillar.sql_database_management:
        if not ch_pillar.sql_user_management:
            raise DbaasClientError('SQL database management is not supported without SQL user management.')
    else:
        ch_pillar.database_names = source_cluster_pillar.database_names

    for format_schema in source_cluster_pillar.format_schemas(''):
        ch_pillar.add_format_schema(format_schema['name'], format_schema['type'], format_schema['uri'])

    for model in source_cluster_pillar.models(''):
        ch_pillar.add_model(model['name'], model['type'], model['uri'])

    for shard_group_name, shard_group in source_cluster_pillar.shard_groups.items():
        shard_group['shard_names'] = [shard_name]
        ch_pillar.add_shard_group(shard_group_name, shard_group)

    return ch_pillar


def _restore_cluster_args(
    s3_buckets: dict,
    target_pillar_id: str,
    source_cluster_id: CID,
    backup: ShardedBackup,
    source_s3_bucket: str,
    source_cloud_storage_s3_bucket: str,
    service_account_id: str,
) -> dict:
    """
    Make restore task args
    """
    return {
        'target-pillar-id': target_pillar_id,
        'restore-from': {
            'cid': source_cluster_id,
            'backups': [
                {
                    'backup-id': backup.id,
                    's3-path': backup.s3_path,
                    'shard-name': backup.shard_names[0],
                }
            ],
        },
        's3_buckets': s3_buckets,
        'source_s3_bucket': source_s3_bucket,
        'source_cloud_storage_s3_bucket': source_cloud_storage_s3_bucket,
        'service_account_id': service_account_id,
    }


def _create_task(cluster: dict, task_args: dict, time_limit: timedelta, source_cid: str, backup_id: str) -> Operation:
    return create_operation(
        task_type=ClickhouseTasks.restore,
        operation_type=ClickhouseOperations.restore,
        metadata=RestoreClusterMetadata(
            source_cid=source_cid,
            backup_id=backup_id,
        ),
        cid=cluster['cid'],
        task_args=task_args,
        time_limit=time_limit,
    )


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.BACKUP, DbaasOperation.RESTORE_HINTS)
def restore_clickhouse_cluster_hints(
    source_cluster: ClusterInfo,
    backup: ShardedBackup,
) -> RestoreHint:
    """
    Compute hints for ClickHouse cluster restore
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_CONSOLE_API')

    backup_shard_name = _get_shard_name_from_backup(backup)
    source_cluster = get_cluster_info_at_time(cluster_id=source_cluster.cid, timestamp=backup.end_time)
    src_ch_resources = _find_source_resource(source_cluster=source_cluster, shard_name=backup_shard_name)
    min_hosts_count = 1

    return RestoreHint(
        resources=ClickHouseRestoreResourcesHint(
            resource_preset_id=src_ch_resources.resource_preset_id,
            disk_size=get_min_restore_disk_size(src_ch_resources),
            min_hosts_count=min_hosts_count,
        ),
        version=None,
        env=source_cluster.env,
        network_id=source_cluster.network_id,
    )
