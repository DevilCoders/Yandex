"""
Time utils
"""
from datetime import datetime, timedelta, timezone
from typing import Optional

from dateutil.parser import parse as _parse_dt_fallback

from .io_limit import calculate_io_limit
from .logs import get_logger
from .types import Backup, ExistedHostResources


def now() -> datetime:
    """
    Return current time with time zone
    """
    return datetime.now(timezone.utc)


def estimate_fetch_time(flavor: dict, size_in_bytes: int) -> timedelta:
    """
    Estimate fetch time
    """
    fetch_bytes_per_second = min(flavor['network_limit'], flavor['io_limit'])
    fetch_seconds = int(size_in_bytes * 1.0 / fetch_bytes_per_second)
    return timedelta(seconds=int(fetch_seconds))


def estimate_logs_apply_time(flavor: dict, size_in_bytes: int, logs_apply_rate: int) -> timedelta:
    """
    Estimate logs (WAL/binlog/oplog) apply time
    logs_apply_rate - log apply speed in bytes/s on one core
    """
    # TODO: we have to estimate logs size somehow
    # but for we assume it's half of storage
    if logs_apply_rate is None or logs_apply_rate == 0:
        raise RuntimeError("Expect logs_apply_rate to be positive integer")
    logs_size_in_bytes = size_in_bytes / 2
    apply_seconds = logs_size_in_bytes / logs_apply_rate / min(flavor['cpu_guarantee'], 2)
    return timedelta(seconds=int(apply_seconds))


def compute_restore_time_limit_2(
    dest_flavor: dict,
    backup: Backup,
    time: datetime,
    src_resources: ExistedHostResources,
    dest_resources: ExistedHostResources,
    retry_count: int = 3,
    replay_factor: float = 0.5,
    software_io_limit: int = None,
) -> timedelta:
    """
    Compute restore task time-limit from flavor and backup duration
    """

    # we cannot rely on flavor.io_limit here because in flavor depends on hosts' disk size in compute.
    io_limit = calculate_io_limit(dest_flavor=dest_flavor, dest_resources=dest_resources)

    if io_limit <= 0:
        raise RuntimeError("io_limit must be positive!")

    if software_io_limit is not None:
        if software_io_limit <= 0:
            raise RuntimeError("software_io_limit must be positive!")
        io_limit = min(software_io_limit, io_limit)

    if time < backup.start_time:
        raise RuntimeError("time must be after backup start!")

    if hasattr(backup, "uncompressed_size") and backup.uncompressed_size is not None:
        size = backup.uncompressed_size
    else:
        size = max(src_resources.disk_size, dest_resources.disk_size)

    restore_time = size / io_limit

    if hasattr(backup, "size") and backup.size is not None and dest_flavor['network_limit'] > 0:
        network_time = backup.size / dest_flavor['network_limit']
        restore_time = max(restore_time, network_time)

    single_restore_time = (
        1.5 * timedelta(seconds=restore_time)
        + replay_factor * (time - backup.start_time)
        + (backup.end_time - backup.start_time)
    )

    return single_restore_time * retry_count


def restore_time_limit(backup_fetch: timedelta, logs_apply: timedelta) -> timedelta:
    """
    Compute restore task time-limit
    """
    # `base` time limit for create cluster task
    base_task_time = timedelta(minutes=30)
    # have to backup cluster after restore (should be as long as backup_fetch)
    # we have 3 tries in worker
    time_limit = ((backup_fetch * 2) + logs_apply + base_task_time) * 3

    get_logger().info(
        'Backup fetch time is %r and time-limit is %r',
        backup_fetch,
        time_limit,
    )
    return time_limit


def compute_restore_time_limit(
    flavor: dict, size_in_bytes: Optional[int], logs_apply_rate: Optional[int] = None
) -> timedelta:
    """
    Compute restore task time-limit from flavor and backup size
    """
    if size_in_bytes is None:
        raise RuntimeError("Expect not None size_in_bytes!")
    backup_fetch = estimate_fetch_time(
        flavor=flavor,
        size_in_bytes=size_in_bytes,
    )
    if logs_apply_rate is not None:
        logs_apply = estimate_logs_apply_time(
            flavor=flavor,
            size_in_bytes=size_in_bytes,
            logs_apply_rate=logs_apply_rate,
        )
    else:
        logs_apply = timedelta(seconds=0)

    if flavor['cpu_fraction'] != 100:
        multiplier = 4
    else:
        multiplier = 1

    return restore_time_limit(backup_fetch, logs_apply) * multiplier


def datetime_to_rfc3339_utcoffset(value: datetime) -> str:
    """
    Convert datetime to string in RFC3999 with UTC offset
    """
    if value.tzinfo is None:
        raise RuntimeError('Got naive datetime. Timezone missing.')
    # convert date to UTC time zone
    dt_at_utc = value.astimezone(timezone.utc)
    if dt_at_utc.microsecond == 0:
        return dt_at_utc.strftime('%Y-%m-%dT%H:%M:%SZ')
    # be fancy like strict-rfc3339: remote trailing zeros
    return dt_at_utc.strftime('%Y-%m-%dT%H:%M:%S.%f').rstrip('0') + 'Z'


def rfc3339_to_datetime(value: str) -> datetime:
    """
    Convert string with RFC3339 to datatime
    """
    # FIXME: when tier0 happens.
    # Changed in version 3.7: When the %z directive is provided to the strptime() method,
    # the UTC offsets can have a colon as a separator between hours
    # minutes and seconds.
    # ....
    # In addition, providing 'Z' is identical to '+00:00'.
    #
    for date_fmt in (
        r'%Y-%m-%dT%H:%M:%S.%fZ',
        r'%Y-%m-%dT%H:%M:%SZ',
    ):
        try:
            parsed_dt = datetime.strptime(value, date_fmt)
        except ValueError:
            continue
        # in our formats timezone specified explicitly
        # with UTC suffix - `Z`.
        # Use .replace(tz) here, cause we don't
        # need time convertion here
        return parsed_dt.replace(tzinfo=timezone.utc)

    parsed_dt = _parse_dt_fallback(value)
    if parsed_dt.tzinfo is None:
        raise ValueError(f'Time zone missing in {value}')
    return parsed_dt
