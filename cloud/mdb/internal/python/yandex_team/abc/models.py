from typing import NamedTuple
from yandex.cloud.priv.team.integration.v1 import abc_service_pb2


class ABCModel(NamedTuple):
    cloud_id: str
    abc_slug: str
    abc_id: int
    default_folder_id: str
    abc_folder_id: str
    _grpc_model = abc_service_pb2.ResolveResponse

    @staticmethod
    def from_api(raw_data) -> 'ABCModel':
        return ABCModel(
            cloud_id=raw_data.cloud_id,
            abc_slug=raw_data.abc_slug,
            abc_id=raw_data.abc_id,
            default_folder_id=raw_data.default_folder_id,
            abc_folder_id=raw_data.abc_folder_id,
        )
