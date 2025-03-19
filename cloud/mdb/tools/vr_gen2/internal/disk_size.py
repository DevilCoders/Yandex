from typing import List, Generator, Tuple, Optional

from humanfriendly import parse_size

from .models.base import DiskSize, DiskRange


def _range_for_custom_disk_sizes(custom_disk_sizes: List[str]) -> List[int]:
    return list(map(lambda s: parse_size(s, binary=True), custom_disk_sizes))


def _range_for_disk_sizes(disk_sizes: DiskRange) -> Generator[int, None, None]:
    start = parse_size(disk_sizes.min, binary=True)
    stop = parse_size(disk_sizes.upto, binary=True)
    step = parse_size(disk_sizes.step, binary=True)
    last = None
    for size in range(start, stop, step):
        last = size
        yield size
    if last != stop and stop % step == 0:
        yield stop


def from_disk_size(d_size: DiskSize) -> Tuple[Optional[dict], Optional[List[int]]]:
    disk_size_range = None
    if d_size.int8range:
        disk_size_range = {'int8range': [d_size.int8range.start, d_size.int8range.end]}
    disk_sizes = None
    if d_size.custom_range:
        disk_sizes = _range_for_disk_sizes(d_size.custom_range)
    elif d_size.custom_sizes:
        disk_sizes = list(_range_for_custom_disk_sizes(d_size.custom_sizes))

    return disk_size_range, disk_sizes
