# coding: utf-8
"""
Restore clusters specific validation
"""

from datetime import datetime, timedelta, timezone

from ..core.exceptions import DbaasClientError, PreconditionFailedError
from .types import Backup, ExistedHostResources, RequestedHostResources


def get_min_restore_disk_size(source: ExistedHostResources) -> int:
    """
    Return minimun disk_size for cluster restore
    """
    return source.disk_size


def assert_can_restore_to_disk(source: ExistedHostResources, dest: RequestedHostResources) -> None:
    """
    Verify that cluster source cluster can be restored into dest
    """
    if dest.disk_size is None:
        raise RuntimeError('Expect dest resource with disk size, got None')
    min_size = get_min_restore_disk_size(source)
    if min_size > dest.disk_size:
        raise PreconditionFailedError(f"Insufficient diskSize, increase it to '{min_size}'")


def assert_cloud_storage_not_turned_off(
    restored_cloud_storage_enabled: bool, source_cloud_storage_enabled: bool
) -> None:
    if not restored_cloud_storage_enabled and source_cloud_storage_enabled:
        raise PreconditionFailedError(
            "Backup with hybrid storage can not be restored to cluster without hybrid storage"
        )


def assert_can_restore_to_time(backup: Backup, restore_to_time: datetime, restore_time_delta: timedelta) -> None:
    """
    Verify that we can use this backup for restore.

    Make sense for database that support PITR
    """
    # Don't allow restore to point in time - it may produce splitbrains
    # add couple minutes reserve for user convinience
    now = datetime.now(timezone.utc) + restore_time_delta
    if restore_to_time > now:
        raise DbaasClientError("It is not possible to restore to future point in time")

    if backup.end_time > restore_to_time:
        # ugly, we can't sent backup.id
        # to client, cause he see global backup id
        # probably we need global-backup-generation
        # to modules level
        raise DbaasClientError(
            f"Unable to restore to '{restore_to_time}' using this backup, "
            f"cause it finished at '{backup.end_time}' "
            "(use older backup or increase 'time')"
        )
