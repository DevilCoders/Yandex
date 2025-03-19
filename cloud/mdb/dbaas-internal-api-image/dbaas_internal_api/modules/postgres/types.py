"""
Types we define for Postgres package
"""
from datetime import datetime
from typing import Any, Dict, List, NamedTuple, Optional

from ...utils.types import EncryptedField, PlainConfigSpec
from ...utils.version import Version
from .constants import MY_CLUSTER_TYPE

PostgresqlRestorePoint = NamedTuple(
    'PostgresqlRestorePoint',
    [('backup_id', str), ('time', datetime), ('time_inclusive', bool), ('restore_latest', bool)],
)


class PostgresqlConfigSpec(PlainConfigSpec):
    """Defines config specs for PostgreSQL"""

    cluster_type = MY_CLUSTER_TYPE
    config_key_prefix = 'postgresql_config'

    def __init__(self, *args, fill_default=True, **kwargs):
        self.fill_default = fill_default
        super().__init__(*args, **kwargs)

    def key(self, key, default=None):
        """Returns given key from configSpec"""
        if default is None:
            default = {}
        return self._data.get(key, default)

    @property
    def backup_start(self) -> Optional[dict]:
        """
        PostgreSQL backup window start
        """
        if self.fill_default:
            return self._data.get('backup_window_start', {})
        return self._data.get('backup_window_start')

    @property
    def retain_period(self) -> Optional[int]:
        """
        PostgreSQL backups retain period
        """
        if self.fill_default:
            return self._data.get('retain_period', 7)
        return self._data.get('retain_period')

    @property
    def max_incremental_steps(self) -> Optional[int]:
        """
        PostgreSQL backups max incremental steps
        """
        if self.fill_default:
            return self._data.get('max_incremental_steps', 6)
        return self._data.get('max_incremental_steps')

    @property
    def use_backup_service(self) -> Optional[bool]:
        """
        PostgreSQL backups retain period
        """
        if self.fill_default:
            return self._data.get('use_backup_service', False)
        return self._data.get('use_backup_service')

    @property
    def access(self) -> Optional[dict]:
        """
        PostgreSQL access options
        """
        if self.fill_default:
            return self._data.get('access', {})
        return self._data.get('access')

    def get_version_strict(self) -> Version:
        """Verify version is set and return it"""
        if self.version is None:
            raise RuntimeError('Unable to get version, it is not defined')

        return self.version


Database = NamedTuple(
    'Database',
    [
        ('name', str),
        ('owner', str),
        ('extensions', List[str]),
        ('lc_collate', str),
        ('lc_ctype', str),
        ('template', str),
    ],
)

ClusterConfig = NamedTuple(
    'ClusterConfig',
    [
        ('version', Version),
        ('pooler_options', Dict[str, Any]),
        ('db_options', Dict[str, Any]),
        ('autofailover', bool),
        ('backup_schedule', Dict),
        ('access', Dict[str, bool]),
        ('perf_diag', Dict[str, Any]),
    ],
)

UserWithPassword = NamedTuple(
    'UserWithPassword',
    [
        ('name', str),
        ('encrypted_password', EncryptedField),
        ('connect_dbs', List[str]),
        ('conn_limit', Optional[int]),
        ('settings', Optional[Dict[str, Any]]),
        ('login', bool),
        ('grants', List[str]),
    ],
)

AuthUser = NamedTuple('AuthUser', [('name', str), ('encrypted_password', EncryptedField), ('dbname', str)])
