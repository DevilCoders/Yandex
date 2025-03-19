# -*- coding: utf-8 -*-
"""
DBaaS Internal API postgresql cluster restore
"""
from copy import deepcopy
from datetime import datetime, timedelta
from typing import Dict, List

from ...core.exceptions import ParseConfigError
from ...core.types import CID, Operation
from ...utils import metadb, validation
from ...utils.alert_group import create_default_alert_group, generate_alert_group_id, get_alerts_by_ag
from ...utils.backup_id import generate_backup_id
from ...utils.backups import ManagedBackupMeta, get_backup_schedule
from ...utils.cluster.get import get_cluster_info_assert_exists, get_cluster_info_at_time, get_subcluster
from ...utils.infra import get_resources_at_rev
from ...utils.logs import log_warn
from ...utils.metadata import RestoreClusterMetadata
from ...utils.metadb import get_cluster_delete_operation, get_cluster_stop_operation, get_managed_alert_group_by_cid
from ...utils.network import validate_security_groups
from ...utils.operation_creator import create_operation
from ...utils.pillar import store_target_pillar
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.restore_validation import (
    assert_can_restore_to_disk,
    assert_can_restore_to_time,
    get_min_restore_disk_size,
)
from ...utils.time import compute_restore_time_limit
from ...utils.types import (
    Alert,
    Backup,
    ClusterDescription,
    ClusterInfo,
    ClusterStatus,
    ExistedHostResources,
    RestoreHintWithTime,
    RestoreResourcesHint,
    VTYPE_PORTO,
    ENV_PROD,
)
from . import create
from .constants import DEFAULT_CONN_LIMIT, MY_CLUSTER_TYPE
from .create import create_cluster
from .pillar import PostgresqlClusterPillar
from .traits import PostgresqlOperations, PostgresqlRoles, PostgresqlTasks
from .types import PostgresqlConfigSpec, PostgresqlRestorePoint, UserWithPassword
from .utils import get_cluster_version_at_time, get_max_connections_limit, get_system_conn_limit

HINT_TIME_ADD = timedelta(minutes=1)
RESTORE_TIME_ADD = timedelta(minutes=2)


def restore_cluster_args(
    target_pillar_id: str,
    source_cluster_id: CID,
    restore_point: PostgresqlRestorePoint,
    s3_bucket: str,
    source_cluster_max_connections: int,
    source_cluster_backup_service_usage: bool = False,
) -> dict:
    """
    Make postgresql_cluster_restore task args
    """
    return {
        's3_buckets': {
            'backup': s3_bucket,
        },
        'target-pillar-id': target_pillar_id,
        'restore-from': {
            'cid': source_cluster_id,
            'backup-id': restore_point.backup_id,
            'time': restore_point.time.isoformat(),
            'time-inclusive': restore_point.time_inclusive,
            'source-cluster-options': {
                'max_connections': source_cluster_max_connections,
            },
            'restore-latest': restore_point.restore_latest,
            # sent it to worker for legacy reasons,
            # cause salt expect this dict
            'rename-database-from-to': {},
            'use_backup_service_at_latest_rev': source_cluster_backup_service_usage,
        },
        'use_backup_service': source_cluster_backup_service_usage,
    }


def get_source_max_connections(source_pillar: PostgresqlClusterPillar, source_resources: ExistedHostResources) -> int:
    """
    Get source cluster max connections
    """
    if source_pillar.config.max_connections is not None:
        return source_pillar.config.max_connections
    # max_connections defined implicitly,
    # calculate it from flavour
    flavor = validation.get_flavor_by_name(source_resources.resource_preset_id)
    return get_max_connections_limit(flavor)


def _transfer_walg_options(source_options: Dict) -> Dict:
    """
    Set some wal-g options if they were set in source cluster
    """
    result = dict()
    options = ['delta_max_steps', 'without_files_metadata']
    for option in options:
        if option in source_options:
            result[option] = source_options[option]
    return result


def _reset_conn_limit(user: UserWithPassword) -> UserWithPassword:
    return user._replace(conn_limit=None)


def _sum_conn_limit(users: List[UserWithPassword]) -> int:
    users_conns = sum(u.conn_limit if u.conn_limit is not None else DEFAULT_CONN_LIMIT for u in users)
    return users_conns + get_system_conn_limit()


def verify_max_conns(users: List[UserWithPassword], dest_flavor: dict, dest_pg_options: dict) -> List[UserWithPassword]:
    """
    Set conn_limit to default if sum(conn_limit) < max_conns
    """
    max_connections = dest_pg_options.get('max_connections')
    if max_connections is None:
        max_connections = get_max_connections_limit(dest_flavor)

    actual_conns = _sum_conn_limit(users)
    if actual_conns > max_connections:
        log_warn(
            'Sum user_conns: %r is greater then cluster max_connections: %r. ' 'Reset conn_limit for users',
            actual_conns,
            max_connections,
        )
        # after reset sum(conn_limit) may not fit max_connections
        # but create vaildate it
        return [_reset_conn_limit(u) for u in users]
    return users


def _is_restore_target_overshooted(source_cluster: ClusterInfo, time: datetime) -> bool:
    """
    Returns True if time is greater than delete time for deleted cluster or stop time for stopped cluster
    """
    if source_cluster.status in (
        ClusterStatus.deleting,
        ClusterStatus.delete_error,
        ClusterStatus.deleted,
        ClusterStatus.metadata_deleting,
        ClusterStatus.metadata_deleted,
        ClusterStatus.metadata_delete_error,
    ):
        delete_operation = get_cluster_delete_operation(source_cluster.cid, PostgresqlOperations)
        if delete_operation is None:
            raise RuntimeError(
                f'Unable to get {source_cluster.cid} cluster (in {source_cluster.status} status) delete operation'
            )
        return time > delete_operation.created_at
    if source_cluster.status in (
        ClusterStatus.stopping,
        ClusterStatus.stop_error,
        ClusterStatus.stopped,
    ):
        stop_operation = get_cluster_stop_operation(source_cluster.cid, PostgresqlOperations)
        if stop_operation is None:
            raise RuntimeError(
                f'Unable to get {source_cluster.cid} cluster (in {source_cluster.status} status) stop operation'
            )
        return time > stop_operation.created_at
    return False


def _combine_cluster_config(new_config: Dict, source_config: Dict) -> Dict:
    """
    Set absent in new_config values from source_config. Iterate by predefined list of config options
    """
    result_config = deepcopy(new_config)
    options = [
        'user_shared_preload_libraries',
    ]
    for option in options:
        if option not in result_config and option in source_config:
            result_config[option] = source_config[option]
    return result_config


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
def restore_postgresql_cluster(
    description: ClusterDescription,
    backup: Backup,
    source_cluster: ClusterInfo,
    time: datetime,
    time_inclusive: bool,
    config_spec: dict,
    network_id: str,
    host_specs: List[dict],
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
    host_group_ids: List[str] = None,
    alert_group_spec: dict = None,
) -> Operation:
    # pylint: disable=too-many-locals
    """
    Create postgresql cluster from backup. Returns task for worker
    """
    try:
        # Load options
        # version not required,
        # cause we get it from source cluster
        pg_config_spec = PostgresqlConfigSpec(config_spec, version_required=False)
    except ValueError as err:
        raise ParseConfigError(err)

    # Check that before get cluster rev,
    # cause user may pass time in which cluster doesn't exist
    assert_can_restore_to_time(backup, time, RESTORE_TIME_ADD)

    source_cluster = get_cluster_info_at_time(source_cluster.cid, time)
    source_cluster_latest = get_cluster_info_assert_exists(source_cluster.cid, MY_CLUSTER_TYPE, include_deleted=True)
    source_resources = get_resources_at_rev(source_cluster.cid, source_cluster.rev)

    dest_resources = pg_config_spec.get_resources()

    dest_flavor = validation.get_flavor_by_name(dest_resources.resource_preset_id)

    # actually here should be logic like:
    #  1. Find maximum database size from base-backup (belongs to our delta-backup)
    #     to our delta backup.
    #  2. 'Calculate' how much space we need for WALs (keep in mind that we prefetch them)
    #  3 ... ???
    #
    # but right now we just compare disk size
    assert_can_restore_to_disk(source_resources, dest_resources)

    source_pillar = PostgresqlClusterPillar(source_cluster.pillar)
    source_version = get_cluster_version_at_time(source_cluster.cid, time)

    pg_users = source_pillar.pgusers
    public_users = pg_users.get_public_users()

    if source_pillar.sox_audit:
        idm_roles = ('reader', 'writer')
        for idm_role in idm_roles:
            pg_users.add_user(
                UserWithPassword(
                    name=idm_role,
                    encrypted_password={},
                    connect_dbs=[],
                    conn_limit=0,
                    settings={},
                    login=False,
                    grants=[],
                )
            )
        public_users = pg_users.get_public_users()
        for user in public_users:
            for idm_role in idm_roles:
                if idm_role in user.grants:
                    user.grants.remove(idm_role)

    dest_pg_options = pg_config_spec.get_config()
    users = verify_max_conns(users=public_users, dest_flavor=dest_flavor, dest_pg_options=dest_pg_options)
    databases = source_pillar.databases.get_databases()

    backup_start = pg_config_spec.key('backup_window_start')
    if not backup_start:
        backup_start = source_cluster.backup_schedule.get('start', {})

    backup_retain_period = pg_config_spec.retain_period
    if not backup_retain_period:
        backup_retain_period = source_cluster.backup_schedule.get('retain_period', 7)

    use_backup_service = source_cluster.backup_schedule.get('use_backup_service', False)

    max_incremental_steps = pg_config_spec.max_incremental_steps
    if not max_incremental_steps:
        max_incremental_steps = source_cluster.backup_schedule.get('max_incremental_steps', 6)

    backup_schedule = get_backup_schedule(
        cluster_type=MY_CLUSTER_TYPE,
        backup_window_start=backup_start,
        backup_retain_period=backup_retain_period,
        use_backup_service=use_backup_service,
        max_incremental_steps=max_incremental_steps,
    )

    dest_cid, dest_pillar = create_cluster(
        description=description,
        host_specs=host_specs,
        databases=databases,
        users=users,
        flavor=dest_flavor,
        network_id=network_id,
        resources=dest_resources,
        pg_config=create.ClusterConfig(
            version=source_version,
            pooler_options=pg_config_spec.key('pooler_config'),
            # If some options (like shared_preload_libraries) were not setted in new cluster
            # we need to copy them from old cluster
            db_options=_combine_cluster_config(dest_pg_options, source_pillar.config.get_config()),
            autofailover=pg_config_spec.key('autofailover', source_pillar.pgsync.get_autofailover()),
            backup_schedule=backup_schedule,
            access=pg_config_spec.key('access', source_pillar.access),
            perf_diag=source_pillar.perf_diag.get(),
        ),
        deletion_protection=deletion_protection,
        host_group_ids=host_group_ids,
        walg_options=_transfer_walg_options(source_pillar.walg.get()),
        sox_audit=pg_config_spec.key(
            'sox_audit',
            dest_flavor['vtype'].lower() == VTYPE_PORTO.lower() and description.environment.lower() == ENV_PROD.lower(),
        ),
        is_restore=True,
    )

    target_pillar_id = store_target_pillar(new_cluster_id=dest_cid, source_cluster_pillar=source_pillar.as_dict())

    source_cluster_backup_service_usage = source_cluster_latest.backup_schedule.get('use_backup_service', False)

    if not source_cluster_backup_service_usage:
        backup_id = backup.id
    else:
        # parse imported backups
        managed_backup = metadb.get_managed_backup(backup.id)

        meta = ManagedBackupMeta.load(managed_backup.metadata)
        backup_id = meta.original_name

    task_args = restore_cluster_args(
        s3_bucket=dest_pillar.s3_bucket,
        target_pillar_id=target_pillar_id,
        source_cluster_id=source_cluster.cid,
        restore_point=PostgresqlRestorePoint(
            backup_id=backup_id,
            time=time,
            time_inclusive=time_inclusive,
            restore_latest=_is_restore_target_overshooted(source_cluster, time),
        ),
        source_cluster_max_connections=get_source_max_connections(
            source_pillar=source_pillar, source_resources=source_resources
        ),
        # MDB-13980 we set flag about source cluster latest rev backup service usage, because it matter in salt state
        # to consider way of resolving backup
        source_cluster_backup_service_usage=source_cluster_backup_service_usage,
    )

    subcid = get_subcluster(role=PostgresqlRoles.postgresql, cluster_id=dest_cid)['subcid']

    backups = [
        {
            "backup_id": generate_backup_id(),
            "subcid": subcid,
            "shard_id": None,
        }
    ]

    task_args['initial_backup_info'] = backups

    if security_group_ids:
        sg_ids = set(security_group_ids)
        validate_security_groups(sg_ids, network_id)
        task_args['security_group_ids'] = list(sg_ids)

    if alert_group_spec:
        ag_id = create_default_alert_group(cid=dest_cid, alert_group_spec=alert_group_spec)
        task_args["default_alert_group_id"] = ag_id
    else:
        source_ag = get_managed_alert_group_by_cid(source_cluster.cid)
        if source_ag:
            ag_id = generate_alert_group_id()

            metadb.add_alert_group(
                cid=dest_cid,
                ag_id=ag_id,
                monitoring_folder_id=source_ag['monitoring_folder_id'],
                managed=False,
            )

            for alert in get_alerts_by_ag(source_ag['alert_group_id']):
                metadb.add_alert_to_group(cid=dest_cid, alert=Alert(**alert), ag_id=ag_id)

            task_args["default_alert_group_id"] = ag_id

    return create_operation(
        task_type=PostgresqlTasks.restore,
        operation_type=PostgresqlOperations.restore,
        metadata=RestoreClusterMetadata(
            source_cid=source_cluster.cid,
            backup_id=backup.id,
        ),
        cid=dest_cid,
        time_limit=compute_restore_time_limit(
            flavor=dest_flavor,
            size_in_bytes=max(source_resources.disk_size, dest_resources.disk_size),  # type: ignore
        ),
        task_args=task_args,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
def restore_postgresql_cluster_hints(
    source_cluster: ClusterInfo,
    backup: Backup,
) -> RestoreHintWithTime:
    """
    Compute hints for PostgreSQL cluster restore
    """
    time = backup.end_time + HINT_TIME_ADD
    source_cluster = get_cluster_info_at_time(source_cluster.cid, time)
    source_resources = get_resources_at_rev(source_cluster.cid, source_cluster.rev)
    version = get_cluster_version_at_time(source_cluster.cid, time)
    return RestoreHintWithTime(
        resources=RestoreResourcesHint(
            resource_preset_id=source_resources.resource_preset_id,
            disk_size=get_min_restore_disk_size(source_resources),
            role=PostgresqlRoles.postgresql,
        ),
        version=version.to_string(),
        env=source_cluster.env,
        time=time,
        network_id=source_cluster.network_id,
    )
