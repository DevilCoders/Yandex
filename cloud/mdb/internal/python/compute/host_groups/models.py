from datetime import datetime
from enum import Enum
from typing import Dict, NamedTuple

from yandex.cloud.priv.compute.v1 import host_group_pb2

from cloud.mdb.internal.python.compute.models import enum_from_api


class HostGroupStatus(Enum):
    UNKNOWN = host_group_pb2.HostGroup.Status.STATUS_UNSPECIFIED
    CREATING = host_group_pb2.HostGroup.Status.CREATING
    READY = host_group_pb2.HostGroup.Status.READY
    UPDATING = host_group_pb2.HostGroup.Status.UPDATING
    DELETING = host_group_pb2.HostGroup.Status.DELETING

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, HostGroupStatus)


class HostGroupModel(NamedTuple):
    id: str
    folder_id: str
    created_at: datetime
    name: str
    description: str
    labels: Dict[str, str]
    zone_id: str
    status: HostGroupStatus
    type_id: str

    @staticmethod
    def from_api(raw_host_group: host_group_pb2.HostGroup):
        return HostGroupModel(
            id=raw_host_group.id,
            folder_id=raw_host_group.folder_id,
            created_at=raw_host_group.created_at.ToDatetime(),
            name=raw_host_group.name,
            description=raw_host_group.description,
            labels=raw_host_group.labels,
            zone_id=raw_host_group.zone_id,
            status=HostGroupStatus.from_api(raw_host_group.status),
            type_id=raw_host_group.type_id,
        )
