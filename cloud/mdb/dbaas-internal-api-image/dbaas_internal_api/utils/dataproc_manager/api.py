"""
Interface to dataproc-manager to be used within application
"""

from abc import ABC, abstractmethod
from typing import Dict, List

from .health import ExtendedClusterHealth
from ...modules.hadoop.traits import HostHealth


class DataprocManagerAPI(ABC):
    """
    Interface to dataproc-manager to be used within application
    """

    @abstractmethod
    def cluster_health(self, cid: str) -> ExtendedClusterHealth:
        """
        Returns health-related info about cluster
        """

    @abstractmethod
    def hosts_health(self, cid: str, fqdns: List[str]) -> Dict[str, HostHealth]:
        """
        Returns health info of cluster's hosts
        """
