"""
DBaaS Internal API MySQL backups
"""

from datetime import datetime, timedelta
from functools import partial
import json

from itertools import groupby
from typing import Iterable

from .constants import MY_CLUSTER_TYPE
from .pillar import MySQLPillar, get_cluster_pillar
from .traits import MySQLOperations, MySQLTasks, MySQLRoles
from .utils import get_cluster_version
from ...core.exceptions import BackupListingError, MalformedBackup
from ...core.types import Operation
from ...utils.backups import get_managed_backups, create_backup_via_backup_service, cluster_backup_from_managed
from ...utils.cluster.get import get_subcluster
from ...utils.metadata import BackupClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.s3 import S3Api, get_s3_api
from ...utils.types import Backup, ClusterInfo, BackupStatus
from ...utils.time import rfc3339_to_datetime


def calculate_backup_prefix(cid: str, mysql_version: str) -> str:
    """
    Return prefix to cluster backups
    """
    # magic number '_005' used in wal-e,
    # and for backward compatibility in wal-g
    # https://github.com/wal-g/wal-g/issues/42
    return 'wal-g/{cid}/{mysql_version}/basebackups_005/'.format(cid=cid, mysql_version=mysql_version)


SENTINEL_SUFFIX = '_backup_stop_sentinel.json'
START_TIME_KEY = 'StartLocalTime'
END_TIME_KEY = 'StopLocalTime'


def get_managed_mysql_backups(cid: str) -> Iterable[Backup]:
    """
    Get backups from MetaDB (Created via Backup API)
    """
    mysql_subcluster = get_subcluster(role=MySQLRoles.mysql, cluster_id=cid)
    backups = get_managed_backups(cid=cid, subcid=mysql_subcluster['subcid'], backup_statuses=[BackupStatus.done])

    for backup in backups:
        yield cluster_backup_from_managed(backup)


def extract_backup_id(s3_object: dict, backups_prefix: str) -> str:
    """
    Extract backup key from s3 key
    """
    try:
        s3_key = s3_object['Key']
    except KeyError:
        raise MalformedBackup('Unexpected s3 object: %r, no key found' % s3_object)

    if not s3_key.startswith(backups_prefix):
        raise MalformedBackup('s3_key %r don\'t starts with prefix %r' % (s3_key, backups_prefix))
    s3_key_without_prefix = s3_key[len(backups_prefix) :]

    # handle sentinel key
    # basebackups_005/stream_..._backup_stop_sentinel.json
    if s3_key_without_prefix.endswith(SENTINEL_SUFFIX):
        return s3_key_without_prefix[: -len(SENTINEL_SUFFIX)]
    # handle data key

    slash_idx = s3_key_without_prefix.find('/')

    if slash_idx == -1:
        raise MalformedBackup('Unexpected object key, %r' % s3_key)
    return s3_key_without_prefix[:slash_idx]


def get_backups_from_s3(s3api: S3Api, backups_prefix: str):
    """
    Convert s3 listing to backup
    """
    try:
        s3_response = s3api.list_objects(prefix=backups_prefix)
    except IOError as exc:
        raise BackupListingError(exc) from exc

    try:
        response_content = s3_response['Contents']
    except KeyError:
        # there are no backups yet
        return

    for backup_id, s3_objects in groupby(response_content, partial(extract_backup_id, backups_prefix=backups_prefix)):

        s3_backup_info = [o for o in s3_objects if o['Key'].endswith(SENTINEL_SUFFIX)]

        if len(s3_backup_info) == 1:
            try:
                with s3api.get_object_body(s3_backup_info[0]['Key']) as body_fd:
                    try:
                        backup_meta = json.load(body_fd)

                        result = Backup(
                            id=backup_id,
                            start_time=_extract_datetime(backup_meta[START_TIME_KEY]),
                            end_time=_extract_datetime(backup_meta[END_TIME_KEY]),
                        )
                        yield result
                    except (ValueError, SyntaxError, KeyError) as exc:
                        raise MalformedBackup('Unable to parse backup meta') from exc
            except IOError as exc:
                raise BackupListingError(exc) from exc


def _extract_datetime(date_str: str) -> datetime:
    parsed_date = rfc3339_to_datetime(date_str)
    if parsed_date.microsecond > 0:
        return parsed_date.replace(microsecond=0) + timedelta(0, 1)

    return parsed_date


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
def mysql_cluster_backups(info: ClusterInfo) -> Iterable[Backup]:
    """
    Returns cluster backups
    """
    if not info.backup_schedule.get('use_backup_service'):
        pillar = MySQLPillar(info.pillar)
        version = get_cluster_version(info.cid, pillar)
        backups_prefix = calculate_backup_prefix(cid=info.cid, mysql_version=version.to_num())

        s3api = get_s3_api()(bucket=MySQLPillar(info.pillar).s3_bucket)
        return get_backups_from_s3(s3api, backups_prefix)

    return get_managed_mysql_backups(cid=info.cid)


@register_request_handler(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.CREATE)
def mysql_create_backup(cluster: ClusterInfo) -> Operation:
    """
    Create backup
    """
    if not cluster.backup_schedule.get('use_backup_service'):
        pillar = get_cluster_pillar(cluster._asdict())
        task_args = {'zk_hosts': pillar.zk_hosts}
        return create_operation(
            task_type=MySQLTasks.backup,
            operation_type=MySQLOperations.backup,
            metadata=BackupClusterMetadata(),
            time_limit=timedelta(hours=24),
            cid=cluster.cid,
            task_args=task_args,
        )

    subcluster = get_subcluster(role=MySQLRoles.mysql, cluster_id=cluster.cid)

    backup_id = create_backup_via_backup_service(
        cid=cluster.cid,
        subcid=subcluster['subcid'],
    )

    return create_operation(
        task_type=MySQLTasks.wait_backup_service,
        operation_type=MySQLTasks.backup,
        metadata=BackupClusterMetadata(),
        time_limit=timedelta(hours=24),
        cid=cluster.cid,
        task_args={
            'backup_ids': [backup_id],
        },
    )
