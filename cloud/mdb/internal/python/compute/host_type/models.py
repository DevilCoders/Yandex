from typing import NamedTuple

from yandex.cloud.priv.compute.v1 import host_type_pb2


class HostTypeModel(NamedTuple):
    id: str
    cores: int
    memory: int
    disks: int
    disk_size: int

    @staticmethod
    def from_api(raw_host_type: host_type_pb2.HostType):
        return HostTypeModel(
            id=raw_host_type.id,
            cores=raw_host_type.cores,
            memory=raw_host_type.memory,
            disks=raw_host_type.disks,
            disk_size=raw_host_type.disk_size,
        )
