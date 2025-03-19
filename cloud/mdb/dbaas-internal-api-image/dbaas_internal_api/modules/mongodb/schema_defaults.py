"""
Describes flavor- or version-dependent defaults.
"""

from ...utils.types import GIGABYTE


def get_cache_size_gb(instance_type: dict, **_) -> float:
    """
    salt['pillar.get']('data:dbaas:flavor:memory_guarantee') \
        / 2.0 / 1024 / 1024 / 1024
    """
    return instance_type['memory_guarantee'] / 2.0 / GIGABYTE
