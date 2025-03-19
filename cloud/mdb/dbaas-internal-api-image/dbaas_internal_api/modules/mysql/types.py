"""
MySQL common types
"""
from ...utils.types import PlainConfigSpec
from .constants import MY_CLUSTER_TYPE
from typing import NamedTuple, Optional


class MysqlConfigSpec(PlainConfigSpec):
    """Defines config specs for Mysql"""

    # check protobuf `ConfigSpec` for message format
    # https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/mdb/mysql/v1/cluster_service.proto

    cluster_type = MY_CLUSTER_TYPE
    config_key_prefix = 'mysql_config'

    def __init__(self, config_spec: dict, version_required: bool = True, fill_default=True) -> None:
        super().__init__(config_spec, version_required=version_required)
        self.fill_default = fill_default
        self.access = self._data.get('access', {})
        self.perf_diag = self._data.get('perf_diag', {})

    def key(self, key, default=None):
        """Returns given key from configSpec"""
        if default is None:
            default = {}
        return self._data.get(key, default)

    @property
    def backup_start(self) -> Optional[dict]:
        """
        MySQL backup window start
        """
        if self.fill_default:
            return self._data.get('backup_window_start', {})
        return self._data.get('backup_window_start')

    @property
    def retain_period(self) -> Optional[int]:
        """
        MySQL backups retain period
        """
        if self.fill_default:
            return self._data.get('retain_period', 7)
        return self._data.get('retain_period')

    @property
    def use_backup_service(self) -> Optional[bool]:
        """
        MySQL backups service usage
        """
        return self._data.get('use_backup_service', False)

    @property
    def sox_audit(self) -> bool:
        return self._data.get('sox_audit', False)


class ManagedBackupMeta(NamedTuple):
    """Class representing backup-service mysql backup metadata"""

    start_time: str
    finish_time: str
    data_size: str

    @classmethod
    def load(cls, meta: dict) -> "ManagedBackupMeta":
        """
        Construct pg meta from raw.
        """
        # pylint: disable=protected-access
        return ManagedBackupMeta(
            start_time=meta['start_time'],
            finish_time=meta['finish_time'],
            data_size=meta['compressed_size'],
        )
