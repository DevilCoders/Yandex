"""
Type definitions specific to Hadoop package.
"""

from typing import Iterator, Optional

from ...utils.types import PlainConfigSpec
from .constants import MASTER_SUBCLUSTER_TYPE, MY_CLUSTER_TYPE


class HadoopSubclusterSpec(PlainConfigSpec):
    """
    Hadoop Subcluste config specification.

    Example data for hadoop subcluster:
    config_spec: {
        "resources": {
            "resource_preset_id": <flavor_id>,
            "disk_type_id": <type_id>,
            "disk_size": <volume_size>
        },
        "assign_public_ip": False,
        "subnet_id": <subnet_id>,
        "hosts_count": <hosts_count>,
        "role": "<hadoop_subcluster_role>"
        "name": "main"
    }
    """

    cluster_type = MY_CLUSTER_TYPE

    def __init__(self, config_spec: dict) -> None:
        super().__init__(config_spec, version_required=False)

    def get_role(self) -> str:
        """
        Returns role of subcluster.
        """
        return self._data.get('role', MASTER_SUBCLUSTER_TYPE)

    def get_hosts_count(self) -> int:
        """
        Returns hosts_count of subcluster.
        """
        return int(self._data.get('hosts_count', 1))

    def is_assign_public_ip(self) -> bool:
        """
        Returns is_assign_public_ip.
        """
        return self._data.get('assign_public_ip', False)

    def get_host_spec(self) -> dict:
        """
        Returns host_spec.
        """
        return {'assign_public': self.is_assign_public_ip()}

    def get_subnet_id(self) -> Optional[str]:
        """
        Returns subnet_id of subcluster.
        Empty subnet_id is possible for console/clusters:estimate request
        """
        return self._data.get('subnet_id')

    def get_name(self) -> str:
        """
        Returns name of subcluster.
        """
        return self._data['name']

    def is_autoscaling(self) -> bool:
        if 'instance_group_config' in self._data:
            autoscaling_config = self._data['instance_group_config'].get('scale_policy', {}).get('auto_scale', {})
            return autoscaling_config.get('max_size', 0) > autoscaling_config.get('initial_size', 0)
        elif 'autoscaling_config' in self._data:
            return self._data['autoscaling_config'].get('max_hosts_count', 0) > self._data['hosts_count']
        return False


class HadoopConfigSpec(PlainConfigSpec):
    """
    Hadoop config spec.

    Example data for hadoop cluster with multiple sub-clusters
    config_spec: {
        "version_id": "1.0",
        "hadoop": {...},
        "subclusters": [
            {
                "resources": {
                    "resource_preset_id": <flavor_id>,
                    "disk_type_id": <type_id>,
                    "disk_size": <volume_size>
                },
                "assign_public_ip": False,
                "name": "main"
            },
            {
                "resources": {
                    "resource_preset_id": <flavor_id>,
                    "disk_type_id": <type_id>,
                    "disk_size": <volume size>
                },
                "name": "data",
                "hosts_count": 5,
                "role": "hadoop_cluster.datanode",
                ...
            }
        ],
        ...
    }
    """

    cluster_type = MY_CLUSTER_TYPE

    def __init__(self, config_spec: dict) -> None:
        super().__init__(config_spec, version_required=False)

    def get_subclusters(self) -> Iterator[HadoopSubclusterSpec]:
        """
        Method returns a iterator of HadoopSubclusters.
        """
        for subcluster in self._data.get('subclusters', []):
            yield HadoopSubclusterSpec(subcluster)
