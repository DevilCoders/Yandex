# -*- coding: utf-8 -*-
"""
DBaaS Internal API mysql cluster restore
"""
from datetime import datetime, timedelta
from dbaas_internal_api.utils import metadb
from typing import List, Dict, Any

from ...core.exceptions import ParseConfigError
from ...core.types import Operation
from ...utils.backup_id import generate_backup_id
from ...utils.types import MEGABYTE
from ...utils import validation
from ...utils.alert_group import create_default_alert_group, generate_alert_group_id, get_alerts_by_ag
from ...utils.backups import ManagedBackupMeta, get_backup_schedule
from ...utils.cluster.get import get_cluster_info_assert_exists, get_cluster_info_at_time, get_subcluster
from ...utils.infra import get_resources_at_rev
from ...utils.metadata import RestoreClusterMetadata
from ...utils.network import validate_security_groups
from ...utils.operation_creator import create_operation
from ...utils.pillar import store_target_pillar
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.restore_validation import (
    assert_can_restore_to_disk,
    assert_can_restore_to_time,
    get_min_restore_disk_size,
)

from ...utils.time import compute_restore_time_limit_2
from ...utils.types import Alert, Backup, ClusterDescription, ClusterInfo, RestoreHintWithTime, RestoreResourcesHint
from .constants import MY_CLUSTER_TYPE
from .create import create_cluster
from .pillar import MySQLPillar
from .traits import MySQLOperations, MySQLRoles, MySQLTasks
from .types import MysqlConfigSpec
from .utils import get_cluster_version_at_time

HINT_TIME_ADD = timedelta(minutes=1)
RESTORE_TIME_ADD = timedelta(minutes=2)


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
def restore_mysql_cluster(
    description: ClusterDescription,
    backup: Backup,
    source_cluster: ClusterInfo,
    time: datetime,
    config_spec: dict,
    host_specs: List[dict],
    network_id: str,
    security_group_ids: List[str] = None,
    deletion_protection: bool = False,
    host_group_ids: List[str] = None,
    alert_group_spec: dict = None,
) -> Operation:
    # pylint: disable=too-many-locals
    """
    Create MySQL cluster from backup. Returns task for worker
    """
    try:
        # Load options
        # version not required,
        # cause we get it from source cluster
        my_config_spec = MysqlConfigSpec(config_spec, version_required=False)
    except ValueError as err:
        raise ParseConfigError(err)

    # Check that before get cluster rev,
    # cause user may pass time in which cluster doesn't exist
    assert_can_restore_to_time(backup, time, RESTORE_TIME_ADD)

    source_cluster = get_cluster_info_at_time(source_cluster.cid, time)
    source_resources = get_resources_at_rev(source_cluster.cid, source_cluster.rev)

    dest_resources = my_config_spec.get_resources()
    dest_flavor = validation.get_flavor_by_name(dest_resources.resource_preset_id)

    assert_can_restore_to_disk(source_resources, dest_resources)

    source_pillar = MySQLPillar(source_cluster.pillar)

    # patch MysqlConfigSpec:
    my_config_spec.version = get_cluster_version_at_time(source_cluster.cid, time, source_pillar)
    my_config_spec.access = my_config_spec.access or source_pillar.access
    my_config_spec.perf_diag = my_config_spec.perf_diag or source_pillar.perf_diag
    cfg_path = 'mysql_config_' + my_config_spec.version.to_string('_')
    my_config_spec.set_config(
        [cfg_path, 'lower_case_table_names'], source_pillar.as_dict()['data']['mysql'].get('lower_case_table_names', 0)
    )
    my_config_spec.set_config(
        [cfg_path, 'innodb_page_size'], source_pillar.as_dict()['data']['mysql'].get('innodb_page_size', 16 * 1024)
    )

    backup_start = my_config_spec.backup_start
    if not backup_start:
        backup_start = source_cluster.backup_schedule.get('start', {})

    # obtaining 'use_backup_service' from latest cluster version
    source_cluster_latest = get_cluster_info_assert_exists(source_cluster.cid, MY_CLUSTER_TYPE, include_deleted=True)
    use_backup_service = source_cluster_latest.backup_schedule.get('use_backup_service', False)

    retain_period = my_config_spec.retain_period
    if not retain_period:
        retain_period = source_cluster.backup_schedule.get('retain_period', 7)

    backup_schedule = get_backup_schedule(
        cluster_type=MY_CLUSTER_TYPE,
        backup_window_start=backup_start,
        use_backup_service=use_backup_service,
        backup_retain_period=retain_period,
    )

    if not use_backup_service:
        backup_id = backup.id
    else:
        # parse imported backups
        managed_backup = metadb.get_managed_backup(backup.id)

        meta = ManagedBackupMeta.load(managed_backup.metadata)
        backup_id = meta.original_name

    dest_cid, dest_pillar = create_cluster(
        description=description,
        host_specs=host_specs,
        database_specs=source_pillar.databases(''),
        user_specs=source_pillar.users('', with_pass=True),
        my_config_spec=my_config_spec,
        flavor=dest_flavor,
        resources=dest_resources,
        network_id=network_id,
        backup_schedule=backup_schedule,
        deletion_protection=deletion_protection,
        host_group_ids=host_group_ids,
    )

    target_pillar_id = store_target_pillar(new_cluster_id=dest_cid, source_cluster_pillar=source_pillar.as_dict())

    task_args: Dict[Any, Any] = {
        'zk_hosts': dest_pillar.zk_hosts,
        's3_buckets': {
            'backup': dest_pillar.s3_bucket,
        },
        'target-pillar-id': target_pillar_id,
        'restore-from': {
            'cid': source_cluster.cid,
            'backup-id': backup_id,
            'time': time.isoformat(),
        },
    }
    if security_group_ids:
        sg_ids = set(security_group_ids)
        validate_security_groups(sg_ids, network_id)
        task_args['security_group_ids'] = list(sg_ids)

    subcid = get_subcluster(role=MySQLRoles.mysql, cluster_id=dest_cid)['subcid']

    backups = [
        {
            "backup_id": generate_backup_id(),
            "subcid": subcid,
            "shard_id": None,
        }
    ]

    task_args['initial_backup_info'] = backups
    task_args['use_backup_service'] = True

    if alert_group_spec:
        ag_id = create_default_alert_group(cid=dest_cid, alert_group_spec=alert_group_spec)
        task_args["default_alert_group_id"] = ag_id
    else:
        source_ag = metadb.get_managed_alert_group_by_cid(source_cluster.cid)
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
        task_type=MySQLTasks.restore,
        operation_type=MySQLOperations.restore,
        metadata=RestoreClusterMetadata(
            source_cid=source_cluster.cid,
            backup_id=backup.id,
        ),
        cid=dest_cid,
        time_limit=compute_restore_time_limit_2(
            dest_flavor=dest_flavor,
            backup=backup,
            time=time,
            src_resources=source_resources,
            dest_resources=dest_resources,  # type: ignore
            software_io_limit=65 * MEGABYTE,
        ),
        task_args=task_args,
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
def restore_mysql_cluster_hints(
    source_cluster: ClusterInfo,
    backup: Backup,
) -> RestoreHintWithTime:
    """
    Compute hints for MySQL cluster restore
    """
    time = backup.end_time + HINT_TIME_ADD
    source_cluster = get_cluster_info_at_time(source_cluster.cid, time)
    source_resources = get_resources_at_rev(source_cluster.cid, source_cluster.rev)
    pillar = MySQLPillar(source_cluster.pillar)
    version = get_cluster_version_at_time(source_cluster.cid, time, pillar)
    return RestoreHintWithTime(
        resources=RestoreResourcesHint(
            resource_preset_id=source_resources.resource_preset_id,
            disk_size=get_min_restore_disk_size(source_resources),
            role=MySQLRoles.mysql,
        ),
        version=version.to_string(),
        env=source_cluster.env,
        time=time,
        network_id=source_cluster.network_id,
    )
