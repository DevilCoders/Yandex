"""
Describes flavor dependent defaults.
"""


def get_mark_cache_size(instance_type: dict, **_) -> float:
    """
    salt['pillar.get']('data:dbaas:flavor:memory_guarantee') / 4
    """
    return int(instance_type['memory_guarantee'] / 4.0)
