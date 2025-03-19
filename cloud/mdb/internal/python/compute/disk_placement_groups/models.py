from typing import NamedTuple
from yandex.cloud.priv.compute.v1 import (
    disk_placement_group_pb2,
)


class DiskPlacementGroupModel(NamedTuple):
    id: str
    _grpc_model = disk_placement_group_pb2.DiskPlacementGroup

    @staticmethod
    def from_api(raw_data) -> 'DiskPlacementGroupModel':
        return DiskPlacementGroupModel(
            id=raw_data.id,
        )
