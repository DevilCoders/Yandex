# -*- coding: utf-8 -*-
"""
DBaaS Internal API ClickHouse cluster modification
"""
from datetime import timedelta
from typing import Any, Optional, Tuple, List

from marshmallow import Schema

from ...core.exceptions import (
    DbaasClientError,
    NoChangesError,
    CloudStorageVersionNotSupportedError,
    PreconditionFailedError,
)
from ...core.types import Operation
from ...utils.feature_flags import ensure_no_feature_flag, has_feature_flag
from ...utils.network import validate_security_groups
from ...utils import metadb
from ...utils.backups import get_backup_schedule
from ...utils.cluster.update import (
    is_downscale,
    is_upscale,
    resources_diff,
)
from ...utils.metadata import ModifyClusterMetadata
from ...utils.modify import update_cluster_resources, update_field
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo, ClusterStatus, LabelsDict, MaintenanceWindowDict, VTYPE_COMPUTE
from ...utils.validation import check_cluster_not_in_status, get_flavor_by_name, validate_service_account
from ...utils.version import (
    Version,
    ensure_downgrade_allowed,
    ensure_version_allowed,
    version_validator,
    get_full_version,
)
from .constants import MY_CLUSTER_TYPE, CH_PACKAGE_NAME
from .pillar import get_subcid_and_pillar
from .shards import update_shard_config
from .traits import ClickhouseOperations, ClickhouseRoles, ClickhouseTasks
from .utils import (
    ClickhouseConfigSpec,
    ch_cores_sum,
    create_operation,
    ensure_version_supported_for_cloud_storage,
    get_hosts,
    get_vtype,
    validate_zk_flavor,
)


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
def modify_clickhouse_cluster(
    cluster: dict,
    _schema: Schema,
    config_spec: dict = None,
    name: str = None,
    description: str = None,
    labels: LabelsDict = None,
    maintenance_window: Optional[MaintenanceWindowDict] = None,
    service_account_id: str = None,
    security_group_ids: List[str] = None,
    deletion_protection: Optional[bool] = None,
    **_kwargs,
) -> Operation:
    """
    Modifies ClickHouse cluster.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_UPDATE_CLUSTER_API')

    if service_account_id is not None:
        if get_vtype(cluster['cid']) != VTYPE_COMPUTE:
            raise DbaasClientError('Service Accounts not supported in porto clusters')
        else:
            validate_service_account(service_account_id)

    changes, task_type, task_args, time_limit = _update_cluster_config(
        cluster=cluster, _schema=_schema, config_spec=config_spec, service_account_id=service_account_id
    )

    if security_group_ids is not None:
        sg_ids = set(security_group_ids)
        if set(cluster.get('user_sgroup_ids', [])) != sg_ids:
            validate_security_groups(sg_ids, cluster['network_id'])
            task_args['security_group_ids'] = list(sg_ids)
            changes = True

    if changes:
        check_cluster_not_in_status(ClusterInfo.make(cluster), ClusterStatus.stopped)
        return create_operation(
            task_type=task_type,
            operation_type=ClickhouseOperations.modify,
            metadata=ModifyClusterMetadata(),
            cid=cluster['cid'],
            time_limit=time_limit,
            task_args=task_args,
        )

    raise NoChangesError()


def _update_cluster_config(
    cluster: dict, _schema: Schema, config_spec: Optional[dict], service_account_id: Optional[str]
) -> Tuple[bool, ClickhouseTasks, dict, timedelta]:
    # pylint: disable=too-many-locals,too-many-branches
    changes = False
    task_type = ClickhouseTasks.modify
    task_args: dict[str, Any] = {
        'restart': bool(_schema.context.get('restart')),
    }
    time_limit = timedelta(hours=3)
    subcid, pillar = get_subcid_and_pillar(cluster['cid'])
    update_pillar = False

    if config_spec:
        spec = ClickhouseConfigSpec(config_spec)

        ch_resources, zk_resources = spec.resources

        if ch_resources.resource_preset_id or zk_resources.resource_preset_id:
            _validate_new_flavors(cluster['cid'], ch_resources.resource_preset_id, zk_resources.resource_preset_id)

        if ch_resources:
            state, _ = update_cluster_resources(
                cluster['cid'], MY_CLUSTER_TYPE, ch_resources, ClickhouseRoles.clickhouse
            )
            changes = state.changes
            task_args.update(state.task_args)
            time_limit += state.time_limit
            if state.changes and pillar.version >= Version(20, 4):
                task_args['restart'] = True

        if zk_resources:
            state, _ = update_cluster_resources(
                cluster['cid'], MY_CLUSTER_TYPE, zk_resources, ClickhouseRoles.zookeeper
            )
            if state.changes:
                reverse_order = task_args.get('reverse_order')
                if reverse_order is not None and reverse_order != state.task_args.get('reverse_order'):
                    raise DbaasClientError(
                        'Upscale of one type of hosts cannot be mixed with downscale' ' of another type of hosts'
                    )
                changes = True
                task_args.update(state.task_args)
                time_limit += state.time_limit

        if spec.version and spec.version != pillar.version:
            if changes:
                raise DbaasClientError('Version update cannot be mixed with update of host resources')

            if not has_feature_flag('MDB_CLICKHOUSE_TESTING_VERSIONS'):
                ensure_version_allowed(MY_CLUSTER_TYPE, spec.version)
            else:
                try:
                    get_full_version(MY_CLUSTER_TYPE, spec.version)
                except RuntimeError:
                    version_validator().ensure_version_exists(CH_PACKAGE_NAME, spec.version)
                pillar.set_testing_repos(True)

            if spec.version < pillar.version:
                ensure_downgrade_allowed(MY_CLUSTER_TYPE, pillar.version, spec.version)
                if pillar.sql_user_management and spec.version < Version(20, 6):
                    raise DbaasClientError('SQL user management is not supported in versions lower than 20.6.')
                if pillar.cloud_storage_enabled:
                    min_supported_version = Version(20, 6)
                    if spec.version < min_supported_version:
                        raise CloudStorageVersionNotSupportedError(min_supported_version)

            if pillar.version < Version(22, 0) and spec.version >= Version(22, 0):
                task_args['update_zero_copy_schema'] = True
            elif pillar.version >= Version(22, 0) and spec.version < Version(22, 0):
                raise PreconditionFailedError(
                    f'Downgrade from version \'{pillar.version}\' to \'{spec.version}\' is not allowed'
                )

            changes = True
            task_type = ClickhouseTasks.upgrade
            pillar.set_version(spec.version)
            update_pillar = True

        if spec.config:
            pillar.update_config(spec.config)

            for shard in metadb.get_shards(cid=cluster['cid'], role=ClickhouseRoles.clickhouse):
                update_shard_config(cluster['cid'], shard, spec.config)

            if spec.config.get('geobase_uri'):
                task_args['update-geobase'] = True

            if spec.config.get('dictionaries') and pillar.version < Version(19, 10):
                task_args['restart'] = True

            if (
                spec.config.get('max_table_size_to_drop') or spec.config.get('max_partition_size_to_drop')
            ) and pillar.version < Version(19, 18):
                task_args['restart'] = True

            if spec.config.get('log_level') and pillar.version >= Version(19, 17):
                task_args['restart'] = True

            changes = True
            update_pillar = True

        if spec.cloud_storage and spec.cloud_storage_enabled != pillar.cloud_storage_enabled:
            if not spec.cloud_storage_enabled:
                raise DbaasClientError('Cloud storage cannot be turned-off after cluster creation.')

            cloud_storage_bucket = 'cloud-storage-' + cluster['cid']
            pillar.set_cloud_storage_bucket(cloud_storage_bucket)
            pillar.set_cloud_storage_enabled(True)
            pillar.set_cloud_storage_data_cache_enabled(spec.cloud_storage_data_cache_enabled)
            pillar.set_cloud_storage_data_cache_max_size(spec.cloud_storage_data_cache_max_size)
            pillar.set_cloud_storage_move_factor(spec.cloud_storage_move_factor)

            task_args['enable_cloud_storage'] = True
            task_args['s3_buckets'] = {'cloud_storage': cloud_storage_bucket}

            changes = True
            update_pillar = True

        if pillar.cloud_storage_enabled:
            ensure_version_supported_for_cloud_storage(pillar.version)

        for field in ('backup_start', 'access', 'admin_password'):
            changed = update_field(field, spec, pillar)
            changes |= changed
            if changed:
                task_args['include-metadata'] = True
                update_pillar = True

        if spec.embedded_keeper is not None and spec.embedded_keeper != pillar.embedded_keeper:
            raise DbaasClientError('Embedded keeper setting cannot be switched after cluster creation.')

        if spec.sql_user_management is not None and spec.sql_user_management != pillar.sql_user_management:
            if not spec.sql_user_management:
                raise DbaasClientError('SQL user management cannot be disabled after it has been enabled.')

            if not pillar.admin_password:
                raise DbaasClientError('Admin password must be specified in order to enable SQL user management.')

            if not pillar.user_management_v2:
                raise DbaasClientError('Unable to enable SQL user management.')

            pillar.update_sql_user_management(True)

            changes = True
            update_pillar = True

        if spec.sql_database_management is not None and spec.sql_database_management != pillar.sql_database_management:
            if not spec.sql_database_management:
                raise DbaasClientError('SQL database management cannot be disabled after it has been enabled.')

            if not pillar.sql_user_management:
                raise DbaasClientError('SQL database management is not supported without SQL user management.')

            if not pillar.user_management_v2:
                raise DbaasClientError('Unable to enable SQL database management.')

            pillar.update_sql_database_management(True)

            changes = True
            update_pillar = True

        if spec.backup_start:
            metadb.add_backup_schedule(
                cluster['cid'],
                get_backup_schedule(
                    cluster_type=MY_CLUSTER_TYPE,
                    backup_window_start=spec.backup_start,
                    use_backup_service=cluster['backup_schedule'].get('use_backup_service'),
                ),
            )

    if service_account_id is not None and service_account_id != pillar.service_account_id():
        pillar.update_service_account_id(service_account_id)
        changes = True
        update_pillar = True

    task_args['service_account_id'] = pillar.service_account_id()

    if update_pillar:
        metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return changes, task_type, task_args, time_limit


def _validate_new_flavors(cid: str, ch_preset_id: Optional[str], zk_preset_id: Optional[str]):
    """
    Check that ZooKeeper hosts have correct flavor in the target cluster configuration.
    """
    ch_hosts, zk_hosts = get_hosts(cid)
    if not zk_hosts:
        return
    ch_cores_total = ch_cores_sum(ch_hosts)
    should_check = False

    if ch_preset_id:
        new_ch_flavor = get_flavor_by_name(ch_preset_id)
        ch_cores_total = new_ch_flavor['cpu_limit'] * len(ch_hosts)
        if is_upscale(resources_diff(ch_hosts, new_ch_flavor)[0]):
            should_check = True

    zk_flavor = metadb.get_flavor_by_id(zk_hosts[0]['flavor'])
    if zk_preset_id:
        zk_flavor = get_flavor_by_name(zk_preset_id)
        if is_downscale(resources_diff(zk_hosts, zk_flavor)[0]):
            should_check = True

    if should_check:
        validate_zk_flavor(ch_cores_total, zk_flavor)
