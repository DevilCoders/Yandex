"""
DBaaS Internal API Redis backups
"""
import json
import os

import aniso8601

from contextlib import suppress
from datetime import timedelta, datetime
from typing import Iterable, Tuple

from ...core.types import Operation
from ...core.exceptions import BackupListingError, MalformedBackup
from ...utils.backups import get_backup_id, get_shard_id
from ...utils.backups import WALG_ROOT_REGEX, is_walg_sentinel_filename, BASE_BACKUP_DIR
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import BackupClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.pillar import get_s3_bucket
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.s3 import S3Api, get_s3_api
from ...utils.types import ClusterInfo, ShardedBackup, BackupInitiator
from .constants import MY_CLUSTER_TYPE
from .traits import RedisOperations, RedisTasks

BACKUP_PREFIX = 'redis-backup'
META_FILES = ['meta.json']
START_TIME_KEY = 'dump_start_time'
END_TIME_KEY = 'dump_finish_time'


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
def redis_backups(cluster: ClusterInfo) -> Iterable[ShardedBackup]:
    """
    Returns Redis backups.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_BACKUPS_API')
    s3_backups = []
    s3api = get_s3_api()(get_s3_bucket(cluster.pillar))
    backups_prefix = calculate_backup_prefix(cid=cluster.cid) + '/'
    shards = get_s3_backups_shards(s3api, backups_prefix)
    if not shards:
        # If no shards found, then no backups
        return iter([])

    for shard_id in shards:
        list_prefix = define_backup_list_prefix(cid=cluster.cid, shard_id=shard_id)
        s3_backups += list(get_s3_backups(s3api, backups_prefix, list_prefix))

    return list(sorted(s3_backups, key=lambda v: v.start_time, reverse=True))


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.CREATE)
def redis_create_backup(cluster: ClusterInfo) -> Operation:
    """
    Create Redis backup.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_BACKUPS_API')
    return create_operation(
        task_type=RedisTasks.backup,
        operation_type=RedisOperations.backup,
        metadata=BackupClusterMetadata(),
        time_limit=timedelta(hours=24),
        cid=cluster.cid,
        task_args={},
    )


def get_s3_backups_shards(s3api: S3Api, backups_prefix: str):
    """
    Get shards list from S3
    """
    s3_response = None
    try:
        s3_response = s3api.list_objects(prefix=backups_prefix, delimiter='/')
        ret = []
        for prefix in s3_response.get('CommonPrefixes', []):
            shard_id = get_shard_id(prefix)
            ret.append(shard_id)
        return ret
    except IOError as exc:
        raise BackupListingError(exc) from exc


def calculate_backup_prefix(cid: str) -> str:
    """
    Return prefix to Redis cluster backups
    """
    return '{}/{}'.format(BACKUP_PREFIX, cid)


def define_backup_list_prefix(cid: str, shard_id: str) -> str:
    """
    Return prefix for listing Redis cluster backups
    """
    return '{}/{}/{}/'.format(calculate_backup_prefix(cid), shard_id, BASE_BACKUP_DIR)


def get_s3_backups(s3api: S3Api, backups_prefix: str, backups_list_prefix: str):
    """
    Convert s3 listing to backup
    """
    try:
        s3_response = s3api.list_objects(prefix=backups_list_prefix)
    except IOError as exc:
        raise BackupListingError(exc) from exc

    for s3_obj in s3_response.get('Contents', []):
        if not is_walg_sentinel_filename(s3_obj['Key']):
            continue

        try:
            with s3api.get_object_body(s3_obj['Key']) as body_fd:
                try:
                    backup_meta = json.load(body_fd)
                except (ValueError, KeyError) as exc:
                    raise MalformedBackup('Unable to parse backup meta') from exc
        except IOError as exc:
            raise BackupListingError(exc) from exc

        ret = generate_sharded_backup_from_metadata(
            backup_meta,
            is_managed=False,
            s3_obj=s3_obj,
            backups_prefix=backups_prefix,
        )
        if ret is not None:
            yield ret


def generate_sharded_backup_from_metadata(
    backup_meta: dict,
    is_managed: bool,
    s3_obj: dict,
    backups_prefix: str,
    backup_id: str = None,
):
    # TODO: fix shard name fetch
    shard_names = []
    with suppress(Exception):
        shard_names.append(backup_meta['UserData']['shard_name'])

    if not is_managed:
        backup_id, backups_root = extract_s3_backup_info(s3_obj, backups_prefix)
    else:
        backups_root = None  # type: ignore

    start_time, end_time = get_backup_start_end_time(backup_meta)
    return ShardedBackup(
        id=backup_id,
        start_time=start_time,
        end_time=end_time,
        shard_names=shard_names,
        s3_path=backups_root,
        tool='wal-g',
        managed_backup=is_managed,
        size=backup_meta.get('DataSize', 0),
        btype=BackupInitiator.manual if backup_meta.get('Permanent') else BackupInitiator.automated,
        name=get_backup_id(backup_id),  # type: ignore
    )


def get_backup_start_end_time(backup_meta) -> Tuple[datetime, datetime]:
    return aniso8601.parse_datetime(backup_meta['StartLocalTime']), aniso8601.parse_datetime(
        backup_meta['FinishLocalTime']
    )


def extract_s3_backup_info(s3_object: dict, backups_prefix: str) -> Tuple[str, str]:
    """
    Extract backup key from s3 key
    """

    try:
        s3_key = s3_object['Key']
    except KeyError:
        raise MalformedBackup('Unexpected s3 object: {}, no key found'.format(s3_object))

    if not s3_key.startswith(backups_prefix):
        raise MalformedBackup('s3_key {} don\'t starts with prefix {}'.format(s3_key, backups_prefix))

    # handle sentinel key
    # shard1/basebackups_005/stream_19700101T000030Z_backup_stop_sentinel.json

    try:
        path_match = WALG_ROOT_REGEX.match(s3_key)
        if path_match is None:
            raise BackupListingError('Can not get backup info: {}'.format(s3_key))

        backup_id = '{shard_id}:{backup_name}'.format(shard_id=path_match['shard'], backup_name=path_match['name'])
        backup_path = os.path.join(backups_prefix, path_match['shard'])
    except (AttributeError, KeyError) as exc:
        raise BackupListingError(exc) from exc

    return backup_id, backup_path
