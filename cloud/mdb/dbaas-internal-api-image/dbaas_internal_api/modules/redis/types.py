"""
Redis common types
"""
from ...utils.types import PlainConfigSpec, RequestedHostResources
from ...utils.config import get_disk_type_id
from ...utils.validation import get_flavor_by_name
from ...utils.version import Version
from .constants import MY_CLUSTER_TYPE
from typing import Optional, NamedTuple


def _resolve_disk_type_id(resources: dict) -> Optional[str]:
    """
    Patch disk_type_id
    """
    disk_type_id = resources.get("disk_type_id")
    resource_preset_id = resources.get("resource_preset_id")
    if disk_type_id is None and resource_preset_id is not None:
        flavor = get_flavor_by_name(resource_preset_id)
        disk_type_id = get_disk_type_id(flavor['vtype'])
    return disk_type_id


class RedisConfigSpec(PlainConfigSpec):
    """Defines config specs for Redis"""

    cluster_type = MY_CLUSTER_TYPE
    config_key_prefix = 'redis_config'
    version: Version

    def __init__(self, config_spec: dict, version_required: bool = True) -> None:
        super().__init__(config_spec, version_required=version_required)
        self.backup_start = self._data.get('backup_window_start', {})
        self.access = self._data.get('access', {})
        config = self._data.get(self.versioned_config_key, {})
        self.client_output_limit_buffer_normal = config.pop('client-output-buffer-limit-normal', {})
        self.client_output_limit_buffer_pubsub = config.pop('client-output-buffer-limit-pubsub', {})

    def get_resources(self, disk_type_id_parser=_resolve_disk_type_id) -> RequestedHostResources:  # type: ignore
        """Returns a Resource object"""
        return super(RedisConfigSpec, self).get_resources(disk_type_id_parser=disk_type_id_parser)


class ManagedBackupMeta(NamedTuple):
    """Class representing backup-service mongodb backup metadata"""

    name: str
    data_size: str
    root_path: str

    @classmethod
    def load(cls, meta: dict) -> "ManagedBackupMeta":
        """
        Construct mongo meta from raw.
        """
        # pylint: disable=protected-access
        return ManagedBackupMeta(
            name=meta['name'],
            data_size=meta['DataSize'],
            root_path=meta['root_path'],
        )
