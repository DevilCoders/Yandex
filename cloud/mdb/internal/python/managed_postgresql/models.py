from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from typing import NamedTuple, List, Optional, Dict

from yandex.cloud.priv.mdb.postgresql.v1 import (
    cluster_pb2,
    database_pb2,
    user_pb2,
)

from cloud.mdb.internal.python.compute.models import enum_from_api


class ClusterStatus(Enum):
    UNKNOWN = cluster_pb2.Cluster.Status.STATUS_UNKNOWN
    CREATING = cluster_pb2.Cluster.Status.CREATING
    RUNNING = cluster_pb2.Cluster.Status.RUNNING
    ERROR = cluster_pb2.Cluster.Status.ERROR
    UPDATING = cluster_pb2.Cluster.Status.UPDATING
    STOPPING = cluster_pb2.Cluster.Status.STOPPING
    STOPPED = cluster_pb2.Cluster.Status.STOPPED
    STARTING = cluster_pb2.Cluster.Status.STARTING

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, ClusterStatus)


class ClusterHealth(Enum):
    UNKNOWN = cluster_pb2.Cluster.Health.HEALTH_UNKNOWN
    ALIVE = cluster_pb2.Cluster.Health.ALIVE
    DEAD = cluster_pb2.Cluster.Health.DEAD
    DEGRADED = cluster_pb2.Cluster.Health.DEGRADED

    @staticmethod
    def from_api(grpc_response):
        return enum_from_api(grpc_response, ClusterHealth)


class Cluster(NamedTuple):
    id: str
    folder_id: str
    created_at: datetime
    name: str
    description: str
    labels: Dict[str, str]
    network_id: str
    status: ClusterStatus
    health: ClusterHealth
    security_group_ids: List[str]

    @staticmethod
    def from_api(grpc_response: cluster_pb2.Cluster):
        return Cluster(
            id=grpc_response.id,
            folder_id=grpc_response.folder_id,
            created_at=grpc_response.created_at.ToDatetime(),
            name=grpc_response.name,
            description=grpc_response.description,
            labels=grpc_response.labels,
            network_id=grpc_response.network_id,
            status=ClusterStatus.from_api(grpc_response.status),
            health=ClusterHealth.from_api(grpc_response.health),
            security_group_ids=grpc_response.security_group_ids,
        )


class Extension(NamedTuple):
    name: str
    version: str

    @staticmethod
    def from_api(grpc_response: database_pb2.Extension):
        return Extension(
            name=grpc_response.name,
            version=grpc_response.version,
        )


class Database(NamedTuple):
    name: str
    cluster_id: str
    owner: str
    lc_collate: str
    lc_ctype: str
    extensions: List[Extension]
    template_db: str

    @staticmethod
    def from_api(grpc_response: database_pb2.Database):
        return Database(
            name=grpc_response.name,
            cluster_id=grpc_response.cluster_id,
            owner=grpc_response.owner,
            lc_collate=grpc_response.lc_collate,
            lc_ctype=grpc_response.lc_ctype,
            extensions=grpc_response.extensions,
            template_db=grpc_response.template_db,
        )


@dataclass
class DatabaseSpec:
    name: str
    owner: str
    lc_collate: Optional[str] = None
    lc_ctype: Optional[str] = None
    extensions: Optional[List[Extension]] = ()
    template_db: Optional[str] = None


class Permission(NamedTuple):
    database_name: str

    @staticmethod
    def from_api(grpc_response: user_pb2.Permission):
        return Permission(
            database_name=grpc_response.database_name,
        )


class User(NamedTuple):
    name: str
    cluster_id: str
    permissions: List[Permission]
    conn_limit: int
    login: bool
    grants: List[str]

    @staticmethod
    def from_api(grpc_response: user_pb2.User):
        return User(
            name=grpc_response.name,
            cluster_id=grpc_response.cluster_id,
            permissions=[Permission.from_api(permission) for permission in grpc_response.permissions],
            conn_limit=grpc_response.conn_limit,
            login=grpc_response.login,
            grants=grpc_response.grants,
        )


@dataclass
class UserSpec:
    name: str
    password: str
    conn_limit: Optional[int] = None
    permissions: Optional[List[Permission]] = ()
    login: Optional[bool] = None
    grants: Optional[List[str]] = ()
