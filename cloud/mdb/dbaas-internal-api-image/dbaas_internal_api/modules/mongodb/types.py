"""
MongoDB common types
"""

from typing import Optional, NamedTuple, List, Tuple
from datetime import datetime, timezone

import aniso8601

from ...core.exceptions import DbaasClientError
from ...utils.types import NestedConfigSpec
from .constants import MY_CLUSTER_TYPE
from .traits import MongoDBRoles


class MongodbConfigSpec(NestedConfigSpec):
    """Defines config specs for Mongodb"""

    cluster_type = MY_CLUSTER_TYPE
    config_key_prefix = 'mongodb_spec'
    valid_roles = [e.name for e in MongoDBRoles]

    def __init__(self, config_spec: dict, version_required: bool = True):
        specs_count = len([k for k in config_spec if k.startswith(self.config_key_prefix)])
        if specs_count < 1 and version_required:
            raise ValueError('MongoDB spec was not specified')

        if specs_count > 1:
            raise ValueError('Multiple MongoDB specs were passed')

        super().__init__(config_spec, version_required=version_required)
        self.feature_compatibility_version = self._data.get('feature_compatibility_version')
        self.backup_start = self._data.get('backup_window_start', {})
        self.backup_retain_period = self._data.get('backup_retain_period_days', None)
        self.access = self._data.get('access', {})
        self.performance_diagnostics = self._data.get('performance_diagnostics', {})


class OplogTimestamp(NamedTuple):
    """
    Oplog record time.
    """

    timestamp: int
    inc: Optional[int] = 1

    @property
    def datetime(self) -> datetime:
        return datetime.fromtimestamp(self.timestamp, timezone.utc)


class ManagedBackupMeta(NamedTuple):
    """Class representing backup-service mongodb backup metadata"""

    name: str
    before_ts: OplogTimestamp
    after_ts: OplogTimestamp
    data_size: str
    shard_names: List[str]
    root_path: str

    @classmethod
    def load(cls, meta: dict) -> "ManagedBackupMeta":
        """
        Construct mongo meta from raw.
        """
        # pylint: disable=protected-access

        before_ts = OplogTimestamp(timestamp=meta['before_ts']['TS'], inc=meta['before_ts']['Inc'])
        after_ts = OplogTimestamp(timestamp=meta['after_ts']['TS'], inc=meta['after_ts']['Inc'])
        if before_ts.timestamp == 0 or after_ts.timestamp == 0:
            before_ts, after_ts = get_backup_interval_ts(meta['raw_meta'])

        return ManagedBackupMeta(
            name=meta['name'],
            before_ts=before_ts,
            after_ts=after_ts,
            data_size=meta['data_size'],
            shard_names=meta['shard_names'],
            root_path=meta['root_path'],
        )

    @property
    def rounded_after_ts(self):
        # after_ts second can contain 'in-backup' operations
        # so we need to ensure that recovery target ts is greater than backup_end ts
        return self.after_ts.timestamp + 1

    @property
    def default_recovery_target(self) -> OplogTimestamp:
        return self.after_ts

    def validate_recovery_target(self, target: OplogTimestamp) -> None:
        if target < self.after_ts:
            raise DbaasClientError(
                f"Unable to restore using this backup, "
                f"recovery target must be greater than or equal to "
                f"{self.rounded_after_ts} "
                f"(use older backup or increase 'recoveryTargetSpec')"
            )

        if target.timestamp > int(datetime.now().timestamp()):
            raise DbaasClientError("Invalid recovery target (in future)")


def get_backup_ts(maj_ts: dict, op_ts: dict, dt: str) -> OplogTimestamp:
    if maj_ts['TS'] != 0:
        return OplogTimestamp(maj_ts['TS'], maj_ts['Inc'])
    if op_ts['TS'] != 0:
        return OplogTimestamp(op_ts['TS'], op_ts['Inc'])
    return OplogTimestamp(int(aniso8601.parse_datetime(dt).timestamp()), 1)


def get_backup_interval_ts(backup_meta: dict) -> Tuple[OplogTimestamp, OplogTimestamp]:
    return get_backup_ts(
        backup_meta['MongoMeta']['Before']['LastMajTS'],
        backup_meta['MongoMeta']['Before']['LastTS'],
        backup_meta['StartLocalTime'],
    ), get_backup_ts(
        backup_meta['MongoMeta']['After']['LastMajTS'],
        backup_meta['MongoMeta']['After']['LastTS'],
        backup_meta['FinishLocalTime'],
    )
