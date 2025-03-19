from enum import Enum


class WellKnownPlatforms(Enum):
    MDB_V1 = 'mdb-v1'
    MDB_V2 = 'mdb-v2'
    MDB_V3 = 'mdb-v3'
    STANDARD_V1 = 'standard-v1'
    STANDARD_V2 = 'standard-v2'
    STANDARD_V3 = 'standard-v3'


NVME_100GiB = 100 * (1024**3)
NVME_368GiB = 368 * (1024**3)

PLATFORM_MAP = {
    WellKnownPlatforms.MDB_V1: NVME_100GiB,
    WellKnownPlatforms.MDB_V2: NVME_100GiB,
    WellKnownPlatforms.MDB_V3: NVME_368GiB,
    WellKnownPlatforms.STANDARD_V1: NVME_100GiB,
    WellKnownPlatforms.STANDARD_V2: NVME_100GiB,
    WellKnownPlatforms.STANDARD_V3: NVME_368GiB,
}


class UnsupportedPlatform(Exception):
    """
    Unknown name of platform provided
    """

    pass


def nvme_chunk_size(platform_map: dict[WellKnownPlatforms, int], platform_name: str) -> int:
    try:
        platform = WellKnownPlatforms(platform_name)
    except ValueError:
        raise UnsupportedPlatform(f'"{platform_name}" is an unknown name for platform')
    if platform not in platform_map:
        available = ','.join((str(key.value) for key in platform_map.keys()))
        raise UnsupportedPlatform(f'"{platform_name}" is not configured (available: {available})')
    return platform_map[platform]
