
from google.rpc import code_pb2
from google.rpc import status_pb2
from google.rpc import error_details_pb2

from yandex.cloud.priv.microcosm.instancegroup.v1.instance_group_service_pb2 import UpdateInstanceGroupRequest


def test_can_import():
    assert status_pb2.Status is not None
    assert code_pb2.CANCELLED is not None
    assert error_details_pb2.BadRequest is not None

    assert UpdateInstanceGroupRequest is not None
