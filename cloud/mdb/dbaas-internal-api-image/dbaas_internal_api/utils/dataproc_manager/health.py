"""
Health-related information about cluster
"""

from typing import NamedTuple
from ..types import ClusterHealth


class ExtendedClusterHealth(NamedTuple):
    health: ClusterHealth
    explanation: str
    hdfs_in_safemode: bool
