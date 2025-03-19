"""
Common backup operations
"""
import json
import re

from datetime import datetime
from typing import Callable, Iterator, List, Optional, TypeVar, NamedTuple

from dateutil.tz import tzutc

from ..core.exceptions import BackupListingError, MalformedBackup
from .config import get_default_backup_schedule
from . import metadb
from .backup_id import generate_backup_id, decode_global_backup_id
from .s3 import S3Api
from .types import (
    Backup,
    ManagedBackup,
    BackupStatus,
    BackupInitiatorFromDB,
    ClusterBackup,
    BackupMethodFromDB,
)

BackupT = TypeVar('BackupT', bound=Backup)
BASE_BACKUP_DIR = "basebackups_005"
WALG_ROOT_REGEX = re.compile(r"^.*/(?P<shard>[^/]+)/" + BASE_BACKUP_DIR + r"/(?P<name>.*)_backup_stop_sentinel\.json")
WALG_SENTINEL_SUFFIX = '_backup_stop_sentinel.json'


def get_date_from_backup_meta(backup_meta: dict, date_key: str, date_format: str = None) -> datetime:
    """
    Get and cast datetime from backup_meta
    """
    if not date_format:
        try:
            date_format = backup_meta['date_fmt']
        except KeyError as exc:
            raise MalformedBackup('Unexpected backup meta') from exc

    date_str_value = backup_meta[date_key]
    date_value = datetime.strptime(date_str_value, date_format)  # type: ignore
    if date_value.tzinfo is None:
        date_value = date_value.replace(tzinfo=tzutc())
    return date_value


def extract_name_from_meta_key(s3_key: str, backup_prefix: str, meta_file: str) -> str:
    """
    Return backup name from meta file key
    """
    key_wo_prefix = s3_key[s3_key.index(backup_prefix) + len(backup_prefix) :]
    key_wo_suffix = key_wo_prefix[: -len(meta_file)]
    return key_wo_suffix.strip('/')


def get_backups(
    s3api: S3Api,
    backup_prefix: str,
    meta_files: List[str],
    start_time_key: str = None,
    end_time_key: str = None,
    backup_factory: Callable[[str, str, dict], Optional[BackupT]] = None,
    depth: int = 1,
    max_depth: int = 1,
) -> Iterator[BackupT]:
    """
    List backups in S3.

    If backup_factory is not provided, the default one will be used. In this
    case, start_time_key and end_time_key are required. Otherwise, they are
    ignored.

    :return iterator to backup objects.
    """

    # pylint: disable=too-many-locals
    def _default_backup_factory(_, backup_id, backup_meta):
        return Backup(
            id=backup_id,
            start_time=get_date_from_backup_meta(backup_meta, start_time_key),
            end_time=get_date_from_backup_meta(backup_meta, end_time_key),
        )

    if not backup_factory:
        assert start_time_key
        assert end_time_key
        backup_factory = _default_backup_factory

    try:
        s3_response = s3api.list_objects(prefix=backup_prefix, delimiter='/')
    except IOError as exc:
        raise BackupListingError(exc) from exc

    for obj in s3_response.get('CommonPrefixes', []):
        meta_file = None
        meta_key = None
        for meta_file in meta_files:
            key = obj['Prefix'] + meta_file
            if s3api.object_exists(key):
                meta_key = key
                break

        if not meta_key:
            if depth < max_depth:
                yield from get_backups(
                    s3api,
                    backup_prefix=obj['Prefix'],
                    meta_files=meta_files,
                    backup_factory=backup_factory,
                    depth=depth + 1,
                    max_depth=max_depth,
                )

            continue

        try:
            with s3api.get_object_body(meta_key) as body_fd:
                try:
                    backup_meta = json.load(body_fd)
                except (ValueError, SyntaxError, KeyError) as exc:
                    raise MalformedBackup('Unable to parse backup meta') from exc
        except IOError as exc:
            raise BackupListingError(exc) from exc

        backup_id = extract_name_from_meta_key(meta_key, backup_prefix, meta_file)
        if not backup_id:
            raise RuntimeError(f'Failed to define Backup.id. s3.Key is {meta_key} and backup_prefix is {backup_prefix}')

        backup = backup_factory(backup_prefix, backup_id, backup_meta)
        if backup:
            yield backup


def get_backup_schedule(
    cluster_type: str,
    backup_window_start: Optional[dict] = None,
    backup_retain_period: Optional[int] = None,
    use_backup_service: Optional[bool] = None,
    max_incremental_steps: Optional[int] = None,
) -> dict:
    """
    Generate backup schedule from default and user-provided values
    """

    schedule = get_default_backup_schedule(cluster_type)

    if backup_window_start:
        schedule['start'] = backup_window_start

    if backup_retain_period:
        schedule['retain_period'] = backup_retain_period

    if max_incremental_steps is not None:
        schedule['max_incremental_steps'] = max_incremental_steps

    if use_backup_service is not None:
        schedule['use_backup_service'] = use_backup_service

    return schedule


def get_managed_backups(
    cid: str,
    subcid: Optional[str] = None,
    shard_id: Optional[str] = None,
    backup_statuses: Optional[List[BackupStatus]] = None,
) -> List[ManagedBackup]:
    """
    Return list of shard objects conforming to ShardSchema.
    """

    backups = metadb.list_backups(
        cid=cid,
        subcid=subcid,
        shard_id=shard_id,
        backup_statuses=backup_statuses,
    )

    return [
        ManagedBackup(
            id=backup['backup_id'],
            cid=backup['cid'],
            subcid=backup['subcid'],
            shard_id=backup['shard_id'],
            method=BackupMethodFromDB.load(raw=backup['method']),
            initiator=BackupInitiatorFromDB.load(raw=backup['initiator']),
            status=BackupStatus(backup['status']),
            metadata=backup['metadata'],
            started_at=backup['started_at'],
            finished_at=backup['finished_at'],
        )
        for backup in backups
    ]


def create_backup_via_backup_service(
    cid: str,
    subcid: Optional[str] = None,
    shard_id: Optional[str] = None,
    backup_method: BackupMethodFromDB = BackupMethodFromDB.full,
) -> str:
    backup_id = generate_backup_id()
    metadb.schedule_backup_for_now(
        backup_id=backup_id,
        cid=cid,
        subcid=subcid,
        shard_id=shard_id,
        backup_method=backup_method,
    )
    return backup_id


def is_walg_sentinel_filename(input_str: str) -> bool:
    """Check filename and returns true if it sentinel file, false otherwise"""
    return input_str.endswith(WALG_SENTINEL_SUFFIX)


def get_cluster_id(global_backup_id: str) -> str:
    """
    Get cluster_id from backup_id

    may raise MalformedBackupId
    """
    return decode_global_backup_id(global_backup_id)[0]


def get_backup_id(global_backup_id: str) -> str:
    """
    Get cluster_id from backup_id

    may raise MalformedBackupId
    """
    return decode_global_backup_id(global_backup_id)[1]


def get_shard_id(prefix_item: dict) -> str:
    """
    Get shard_id from item

    converts path:
    redis-backup/cid1/shard1_id/ -> shard1_id
    mongodb-backup/cid1/shard1_id/ -> shard1_id

    :param prefix_item: - one item in s3_response.get('CommonPrefixes', [])
    """
    #
    full_prefix_path = prefix_item.get('Prefix', '')
    stripped = full_prefix_path.strip('/')
    items = stripped.split('/')
    if len(items) < 3:
        raise BackupListingError("Invalid prefix was provided")
    return items[-1]


class ManagedBackupMeta(NamedTuple):
    """Class representing backup-service backup metadata"""

    start_time: str
    finish_time: str
    data_size: str
    uncompressed_data_size: str
    original_name: Optional[str]

    @classmethod
    def load(cls, meta: dict) -> "ManagedBackupMeta":
        """
        Construct backup meta from raw.
        """
        # pylint: disable=protected-access
        return ManagedBackupMeta(
            start_time=meta['start_time'],
            finish_time=meta['finish_time'],
            data_size=meta['compressed_size'],
            uncompressed_data_size=meta.get('uncompressed_size', None),
            original_name=meta.get('name'),
        )


def cluster_backup_from_managed(backup: ManagedBackup) -> ClusterBackup:
    """
    Generic code for backup listing via Backup API
    """
    meta = ManagedBackupMeta.load(backup.metadata)

    return ClusterBackup(
        backup.id,
        start_time=backup.started_at,
        end_time=backup.finished_at,
        managed_backup=True,
        name=backup.id,
        size=meta.data_size,
        method=backup.method,
        uncompressed_size=meta.uncompressed_data_size,
        btype=backup.initiator,
    )
