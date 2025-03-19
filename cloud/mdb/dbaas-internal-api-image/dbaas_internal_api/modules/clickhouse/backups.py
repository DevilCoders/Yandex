# -*- coding: utf-8 -*-
"""
DBaaS Internal API ClickHouse backups
"""
import os
from contextlib import suppress
from datetime import timedelta
from typing import Iterator, Optional

from .pillar import get_pillar
from ...core.exceptions import CloudStorageBackupsNotSupportedError
from ...core.types import Operation
from ...utils.backups import get_backups, get_date_from_backup_meta
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import BackupClusterMetadata
from ...utils.pillar import get_s3_bucket
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.s3 import get_s3_api
from ...utils.types import ClusterInfo, ShardedBackup
from .constants import MY_CLUSTER_TYPE
from .traits import ClickhouseOperations, ClickhouseTasks
from .utils import create_operation
from ...utils.version import Version

META_FILES = ['backup_light_struct.json', 'backup_struct.json']
META_KEY = 'meta'
START_TIME_KEY = 'start_time'
END_TIME_KEY = 'end_time'


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
def clickhouse_backups(cluster: ClusterInfo) -> Iterator[ShardedBackup]:
    """
    Returns cluster backups
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_BACKUPS_API')

    s3api = get_s3_api()(get_s3_bucket(cluster.pillar))
    backup_prefix = 'ch_backup/{}/'.format(cluster.cid)
    return get_backups(s3api, backup_prefix, META_FILES, backup_factory=_make_backup, max_depth=2)


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.CREATE)
def clickhouse_create_backup(cluster: ClusterInfo) -> Operation:
    """
    Create backup
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_BACKUPS_API')

    pillar = get_pillar(cluster.cid)
    if pillar.cloud_storage_enabled and pillar.version < Version(21, 6):
        raise CloudStorageBackupsNotSupportedError()

    return create_operation(
        cid=cluster.cid,
        task_type=ClickhouseTasks.backup,
        operation_type=ClickhouseOperations.backup,
        metadata=BackupClusterMetadata(),
        time_limit=timedelta(hours=24),
    )


def _make_backup(backup_prefix: str, backup_id: str, backup_meta: dict) -> Optional[ShardedBackup]:
    """
    Create ClickhouseBackup from backup id and metadata.
    """
    meta = backup_meta[META_KEY]

    if meta['state'] != 'created':
        return None

    shard_names = []
    with suppress(Exception):
        shard_names.append(meta['labels']['shard_name'])

    return ShardedBackup(
        id=backup_id,
        start_time=get_date_from_backup_meta(meta, START_TIME_KEY),
        end_time=get_date_from_backup_meta(meta, END_TIME_KEY),
        shard_names=shard_names,
        s3_path=os.path.join(backup_prefix, backup_id),
    )
