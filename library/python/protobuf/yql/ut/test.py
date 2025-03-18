from library.python.protobuf.yql import yql_proto_field
from library.python.protobuf.yql.ut.protos.a_pb2 import TMessage
import pytest


@pytest.mark.parametrize("lists_optional", [True, False])
def test_yql_proto_field(lists_optional):
    return yql_proto_field(TMessage, lists_optional=lists_optional)
