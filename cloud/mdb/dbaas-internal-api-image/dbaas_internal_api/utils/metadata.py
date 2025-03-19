"""
Operations metadata

Create you own in module
if you need to add new fields
"""
from abc import ABC, abstractmethod
from datetime import datetime
from typing import List

from .time import datetime_to_rfc3339_utcoffset
from psycopg2.extensions import ISQLQuote
from psycopg2.extras import Json


class Metadata(ABC):
    """
    Base Metdata
    """

    def __conform__(self, proto):
        """
        Tell psycopg2 that we this type
        know how it should be adapted
        """
        if proto is ISQLQuote:
            return self

    @abstractmethod
    def _asdict(self) -> dict:
        pass

    def getquoted(self):
        """
        Convert metadata to db object
        """
        # Expect that Json
        # adapter know how convert UUID
        return Json(self._asdict()).getquoted()


class _ClusterMetadata(Metadata):
    """
    Currently empty metadata

    cid we can get from worker_queue
    """

    def _asdict(self) -> dict:
        """
        Return empty dict,
        cause we don't need to store `cid`,
        we have it in dbaas.worker_queue
        """
        return {}


CreateClusterMetadata = _ClusterMetadata
DeleteClusterMetadata = _ClusterMetadata
ModifyClusterMetadata = _ClusterMetadata
BackupClusterMetadata = _ClusterMetadata
StartClusterMetadata = _ClusterMetadata
StopClusterMetadata = _ClusterMetadata
StartClusterFailoverMetadata = _ClusterMetadata
RebalanceClusterMetadata = _ClusterMetadata


class MoveClusterMetadata(Metadata):
    """
    Move cluster

    we need to store source and destination folder_ext_ids
    """

    def __init__(self, source_folder_id: str, destination_folder_id: str) -> None:
        self.source_folder_id = source_folder_id
        self.destination_folder_id = destination_folder_id

    def _asdict(self) -> dict:
        return {
            'source_folder_id': self.source_folder_id,
            'destination_folder_id': self.destination_folder_id,
        }


class RestoreClusterMetadata(Metadata):
    """
    Restore cluster

    Right now backup.ids are local,
    thats why need source cluster cid
    """

    def __init__(self, source_cid: str, backup_id: str) -> None:
        self.source_cid = source_cid
        self.backup_id = backup_id

    def _asdict(self) -> dict:
        return {'source_cid': self.source_cid, 'backup_id': self.backup_id}


class RescheduleMaintenanceMetadata(Metadata):
    def __init__(self, delayed_until: datetime):
        self.delayed_until = delayed_until

    def _asdict(self) -> dict:
        return {'delayed_until': datetime_to_rfc3339_utcoffset(self.delayed_until)}


class _SubclusterMetadata(Metadata):
    """
    Subcluster metadata
    """

    def _asdict(self) -> dict:
        return {}


CreateSubclusterMetadata = _SubclusterMetadata
DeleteSubclusterMetadata = _SubclusterMetadata


class _HostsMetadata(Metadata):
    """
    Hosts modify metadata
    """

    def __init__(self, host_names: List[str]) -> None:
        self.host_names = host_names

    def _asdict(self) -> dict:
        return {'host_names': self.host_names}


AddClusterHostsMetadata = _HostsMetadata
DeleteClusterHostsMetadata = _HostsMetadata
ModifyClusterHostsMetadata = _HostsMetadata


class _UserMetadata(Metadata):
    """
    User metadata
    """

    def __init__(self, user_name: str) -> None:
        self.user_name = user_name

    def _asdict(self) -> dict:
        return {'user_name': self.user_name}


CreateUserMetadata = _UserMetadata
ModifyUserMetadata = _UserMetadata
DeleteUserMetadata = _UserMetadata

GrantUserPermissionMetadata = _UserMetadata
RevokeUserPermissionMetadata = _UserMetadata


class _DatabaseMetadata(Metadata):
    """
    Database metadata
    """

    def __init__(self, database_name: str) -> None:
        self.database_name = database_name

    def _asdict(self) -> dict:
        return {'database_name': self.database_name}


CreateDatabaseMetadata = _DatabaseMetadata
DeleteDatabaseMetadata = _DatabaseMetadata
ModifyDatabaseMetadata = _DatabaseMetadata


class _ShardMetadata(Metadata):
    """
    Shard metadata.
    """

    def __init__(self, shard_name: str) -> None:
        self.shard_name = shard_name

    def _asdict(self) -> dict:
        return {'shard_name': self.shard_name}


CreateShardMetadata = _ShardMetadata
ModifyShardMetadata = _ShardMetadata
DeleteShardMetadata = _ShardMetadata


class _AlertGroupMetadata(Metadata):
    """
    Alert group metadata.
    """

    def __init__(self, alert_group_id: str) -> None:
        self.alert_group_id = alert_group_id

    def _asdict(self) -> dict:
        return {'alert_group_id': self.alert_group_id}


AlertsGroupClusterMetadata = _AlertGroupMetadata
