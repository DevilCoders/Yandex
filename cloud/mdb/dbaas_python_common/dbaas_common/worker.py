"""
Common utilities for worker
"""

from datetime import timedelta


def get_expected_resize_hours(disk_size: int, host_count: int, min_seconds: int = 0) -> timedelta:
    """
    Calculate expected time for task if all hosts should be transfered
    """
    # excpect 1 GB to be transfered in 60 seconds
    ret = timedelta(hours=(disk_size // (1024**3) * 60 // 3600 * host_count))
    if ret.total_seconds() > min_seconds:
        return ret
    return timedelta(seconds=min_seconds)
