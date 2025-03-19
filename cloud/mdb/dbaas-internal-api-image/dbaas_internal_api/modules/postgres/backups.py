# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL backups
"""

from datetime import timedelta

from typing import Iterable

from dbaas_internal_api.utils.backups import get_backups, cluster_backup_from_managed
from .constants import MY_CLUSTER_TYPE
from .traits import PostgresqlOperations, PostgresqlRoles, PostgresqlTasks
from .utils import get_cluster_version
from ...core.types import Operation
from ...utils.backups import create_backup_via_backup_service, get_managed_backups
from ...utils.cluster.get import get_subcluster
from ...utils.metadata import BackupClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.pillar import get_s3_bucket
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.s3 import get_s3_api
from ...utils.types import Backup, ClusterInfo, BackupStatus

META_FILES = ['metadata.json']
START_TIME_KEY = 'start_time'
END_TIME_KEY = 'finish_time'


def calculate_backup_prefix(cid: str, pg_version: str) -> str:
    """
    Return prefix to cluster backups
    """
    # magic number '_005' used in wal-e,
    # and for backward compatibility in wal-g
    # https://github.com/wal-g/wal-g/issues/42
    return 'wal-e/{cid}/{pg_version}/basebackups_005/'.format(cid=cid, pg_version=pg_version)


def get_managed_postgresql_backups(cid: str) -> Iterable[Backup]:
    """
    Get backups from MetaDB (Created via Backup API)
    """
    postgres_subcluster = get_subcluster(role=PostgresqlRoles.postgresql, cluster_id=cid)
    backups = get_managed_backups(cid=cid, subcid=postgres_subcluster['subcid'], backup_statuses=[BackupStatus.done])
    for backup in backups:
        yield cluster_backup_from_managed(backup)


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
def postgresql_cluster_backups(info: ClusterInfo) -> Iterable[Backup]:
    """
    Returns cluster backups
    """
    if not info.backup_schedule.get('use_backup_service'):
        pg_version = get_cluster_version(info.cid).to_num()

        backup_prefix = calculate_backup_prefix(cid=info.cid, pg_version=pg_version)
        s3api = get_s3_api()(get_s3_bucket(info.pillar))
        return get_backups(
            s3api,
            backup_prefix=backup_prefix,
            meta_files=META_FILES,
            start_time_key=START_TIME_KEY,
            end_time_key=END_TIME_KEY,
        )

    return get_managed_postgresql_backups(cid=info.cid)


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.CREATE)
def postgresql_create_backup(cluster: ClusterInfo) -> Operation:
    """
    Create backup
    """
    if not cluster.backup_schedule.get('use_backup_service'):
        task_args = {
            'zk_hosts': cluster.pillar['data']['pgsync']['zk_hosts'],
        }

        return create_operation(
            task_type=PostgresqlTasks.backup,
            operation_type=PostgresqlOperations.backup,
            metadata=BackupClusterMetadata(),
            time_limit=timedelta(hours=24),
            cid=cluster.cid,
            task_args=task_args,
        )

    subcluster = get_subcluster(role=PostgresqlRoles.postgresql, cluster_id=cluster.cid)

    backup_id = create_backup_via_backup_service(
        cid=cluster.cid,
        subcid=subcluster['subcid'],
    )

    return create_operation(
        task_type=PostgresqlTasks.wait_backup_service,
        operation_type=PostgresqlOperations.backup,
        metadata=BackupClusterMetadata(),
        time_limit=timedelta(hours=24),
        cid=cluster.cid,
        task_args={
            'backup_ids': [backup_id],
        },
    )
