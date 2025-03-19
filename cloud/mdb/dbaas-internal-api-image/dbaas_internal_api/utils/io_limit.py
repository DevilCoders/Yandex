"""
IO limit calculation
"""

from math import ceil
from typing import NamedTuple
from dbaas_internal_api.utils.types import (
    DTYPE_LOCAL_SSD,
    DTYPE_NETWORK_HDD,
    DTYPE_NETWORK_SSD,
    DTYPE_NRD,
    ExistedHostResources,
    GIGABYTE,
    MEGABYTE,
    VTYPE_COMPUTE,
)


class DiskProps(NamedTuple):
    block_size: int
    # max IO for single disk block (for disk = block_count * block_IO)
    io_block: int
    # max IO for whole disk
    io_disk: int


dict_disk_props = {
    (VTYPE_COMPUTE, DTYPE_NETWORK_SSD): DiskProps(32 * GIGABYTE, 15 * MEGABYTE, 450 * MEGABYTE),
    (VTYPE_COMPUTE, DTYPE_NETWORK_HDD): DiskProps(256 * GIGABYTE, 30 * MEGABYTE, 240 * MEGABYTE),
    (VTYPE_COMPUTE, DTYPE_NRD): DiskProps(93 * GIGABYTE, 82 * MEGABYTE, GIGABYTE),
    (VTYPE_COMPUTE, DTYPE_LOCAL_SSD): 500 * MEGABYTE,
}


def compute_from_disk_props(disk_size: int, props: DiskProps) -> int:
    """
    Compute IO limit based on vtype and disk type/size
    """
    return min(ceil(disk_size / props.block_size) * props.io_block, props.io_disk)


def calculate_io_limit(dest_flavor: dict, dest_resources: ExistedHostResources) -> int:
    """
    Compute IO limit
    """
    props = dict_disk_props.get((dest_flavor["vtype"], dest_resources.disk_type_id))

    if props is None:
        # get from flavor by default
        return dest_flavor['io_limit']
    if isinstance(props, int):
        return props
    if isinstance(props, DiskProps):
        return compute_from_disk_props(dest_resources.disk_size, props)
    raise RuntimeError(f'calculate_io_limit got unexpected {props=}')
