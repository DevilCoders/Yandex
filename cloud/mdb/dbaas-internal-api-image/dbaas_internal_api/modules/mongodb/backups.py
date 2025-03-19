# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB backups
"""
import json
import os
from contextlib import suppress
from datetime import timedelta
from typing import Iterator, Tuple

from .types import ManagedBackupMeta, get_backup_interval_ts
from ...core.exceptions import BackupListingError, MalformedBackup
from ...core.types import Operation
from ...utils.backups import create_backup_via_backup_service, get_managed_backups, get_backup_id, get_shard_id
from ...utils.backups import WALG_ROOT_REGEX, is_walg_sentinel_filename, BASE_BACKUP_DIR
from ...utils.cluster.get import get_subcluster, get_shards
from ...utils.metadata import BackupClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.pillar import get_s3_bucket
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.s3 import S3Api, get_s3_api
from ...utils.types import ClusterInfo, ShardedBackup, BackupInitiator, BackupStatus, ManagedBackup
from .constants import MONGOCFG_HOST_TYPE, MONGOINFRA_HOST_TYPE, MY_CLUSTER_TYPE, SUBCLUSTER_NAMES
from .traits import MongoDBOperations, MongoDBRoles, MongoDBTasks


def calculate_backup_prefix(cid: str) -> str:
    """
    Return prefix to MongoDB cluster backups
    """
    return 'mongodb-backup/{}/'.format(cid)


def define_backup_list_prefix(cid: str, shard_id: str) -> str:
    """
    Return prefix for listing MongoDB cluster backups
    """
    return 'mongodb-backup/{}/{}/{}/'.format(cid, shard_id, BASE_BACKUP_DIR)


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
def mongodb_backups(cluster: ClusterInfo) -> Iterator[ShardedBackup]:
    """
    Returns cluster backups
    """

    if not cluster.backup_schedule.get('use_backup_service'):
        s3_backups = []
        s3api = get_s3_api()(get_s3_bucket(cluster.pillar))
        backups_prefix = calculate_backup_prefix(cid=cluster.cid)
        shards = get_s3_backups_shards(s3api, backups_prefix)
        if not shards:
            # If no shards found, then no backups
            return iter([])
        for shard_id in shards:
            list_prefix = define_backup_list_prefix(cid=cluster.cid, shard_id=shard_id)
            s3_backups += list(get_s3_backups(s3api, backups_prefix, list_prefix))

        return iter(sorted(s3_backups, key=lambda v: v.start_time, reverse=True))

    return get_managed_mongodb_backups(cid=cluster.cid)


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.CREATE)
def mongodb_create_backup(cluster: ClusterInfo) -> Operation:
    """
    Create backup
    """
    if not cluster.backup_schedule.get('use_backup_service', None):
        return create_operation(
            task_type=MongoDBTasks.backup,
            operation_type=MongoDBOperations.backup,
            metadata=BackupClusterMetadata(),
            time_limit=timedelta(hours=24),
            cid=cluster.cid,
            task_args={},
        )

    backup_ids = []
    try:
        mongocfg_subcluster = get_subcluster(role=MongoDBRoles.mongocfg, cluster_id=cluster.cid)
        backup_id = create_backup_via_backup_service(
            cid=cluster.cid,
            subcid=mongocfg_subcluster['subcid'],
            shard_id=None,
        )
        backup_ids.append(backup_id)
    except RuntimeError:
        pass

    mongod_subcluster = get_subcluster(role=MongoDBRoles.mongod, cluster_id=cluster.cid)
    for shard in get_shards({'cid': cluster.cid}, role=MongoDBRoles.mongod):
        backup_id = create_backup_via_backup_service(
            cid=cluster.cid,
            subcid=mongod_subcluster['subcid'],
            shard_id=shard['shard_id'],
        )
        backup_ids.append(backup_id)

    return create_operation(
        task_type=MongoDBTasks.wait_backup_service,
        operation_type=MongoDBOperations.backup,
        metadata=BackupClusterMetadata(),
        time_limit=timedelta(hours=24),
        cid=cluster.cid,
        task_args={
            'backup_ids': backup_ids,
        },
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

    if SUBCLUSTER_NAMES[MONGOCFG_HOST_TYPE] in shard_names or SUBCLUSTER_NAMES[MONGOINFRA_HOST_TYPE] in shard_names:
        return None

    if not is_managed:
        backup_id, backups_root = extract_s3_backup_info(s3_obj, backups_prefix)
    else:
        backups_root = None  # type: ignore

    raw_before_ts, raw_after_ts = get_backup_interval_ts(backup_meta)
    return ShardedBackup(
        id=backup_id,
        start_time=raw_before_ts.datetime,
        end_time=raw_after_ts.datetime,
        shard_names=shard_names,
        s3_path=backups_root,
        tool='wal-g',
        managed_backup=is_managed,
        size=backup_meta.get('DataSize', 0),
        btype=BackupInitiator.manual if backup_meta.get('Permanent') else BackupInitiator.automated,
        name=get_backup_id(backup_id),  # type: ignore
    )


def sharded_backup_from_managed(backup: ManagedBackup) -> ShardedBackup:
    meta = ManagedBackupMeta.load(backup.metadata)
    return ShardedBackup(
        id=backup.id,
        start_time=meta.before_ts.datetime,
        end_time=meta.after_ts.datetime,
        shard_names=meta.shard_names,
        s3_path=meta.root_path,
        tool='wal-g',
        size=meta.data_size,
        btype=backup.initiator,
        name=meta.name,
    )


def get_managed_mongodb_backups(cid: str) -> Iterator[ShardedBackup]:
    """
    Get backups from MetaDB (Created via Backup API)
    """
    mongod_subcluster = get_subcluster(role=MongoDBRoles.mongod, cluster_id=cid)
    backups = get_managed_backups(cid=cid, subcid=mongod_subcluster['subcid'], backup_statuses=[BackupStatus.done])
    for backup in backups:
        yield sharded_backup_from_managed(backup)
