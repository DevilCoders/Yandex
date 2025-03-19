from typing import NamedTuple
from yandex.cloud.priv.compute.v1 import (
    placement_group_pb2,
)


class PlacementGroupModel(NamedTuple):
    id: str
    _grpc_model = placement_group_pb2.PlacementGroup

    @staticmethod
    def from_api(raw_data) -> 'PlacementGroupModel':
        return PlacementGroupModel(
            id=raw_data.id,
        )
