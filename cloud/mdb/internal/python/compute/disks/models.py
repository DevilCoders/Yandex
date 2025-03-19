import copy
from datetime import datetime
from enum import Enum
from typing import Any, Dict, NamedTuple, List

from yandex.cloud.priv.compute.v1 import (
    disk_pb2,
)

from cloud.mdb.internal.python.compute.models import OperationModel, enum_from_api


class State(Enum):
    STATUS_UNSPECIFIED = disk_pb2.Disk.Status.STATUS_UNSPECIFIED
    CREATING = disk_pb2.Disk.Status.CREATING
    READY = disk_pb2.Disk.Status.READY
    ERROR = disk_pb2.Disk.Status.ERROR
    DELETING = disk_pb2.Disk.Status.DELETING

    @staticmethod
    def from_api(raw_data):
        return enum_from_api(raw_data, State)


class DiskModel(NamedTuple):
    id: str
    folder_id: str
    name: str
    size: int
    zone_id: str
    type_id: str
    created_at: datetime
    instance_ids: List[str]
    status: State

    @staticmethod
    def from_api(raw_data: disk_pb2.Disk) -> 'DiskModel':
        return DiskModel(
            id=raw_data.id,
            folder_id=raw_data.folder_id,
            name=raw_data.name,
            size=raw_data.size,
            zone_id=raw_data.zone_id,
            type_id=raw_data.type_id,
            created_at=raw_data.created_at.ToDatetime(),
            instance_ids=list(raw_data.instance_ids),
            status=State.from_api(raw_data.status),
        )

    def __repr__(self):
        return f'DiskModel id={self.id}'

    def to_json(self) -> Dict[str, Any]:
        js = self._asdict()
        js['created_at'] = self.created_at.isoformat()
        js['status'] = self.status.name
        return js

    @staticmethod
    def from_json(js: Dict[str, Any]) -> 'DiskModel':
        js_copy = copy.deepcopy(js)
        js_copy['created_at'] = datetime.fromisoformat(js_copy['created_at'])
        js_copy['status'] = State[js_copy['status']]
        return DiskModel(**js_copy)


class CreatedDisk(OperationModel):
    disk_id: str

    def __init__(self, disk_id: str):
        self.disk_id = disk_id


class DiskType(Enum):
    network_ssd_nonreplicated = 'network-ssd-nonreplicated'
    network_hdd = 'network-hdd'
    network_ssd = 'network-ssd'
    local_ssd = 'local-ssd'
