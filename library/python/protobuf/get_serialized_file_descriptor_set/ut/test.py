import base64

from library.python.protobuf.get_serialized_file_descriptor_set import get_serialized_file_descriptor_set
from library.python.protobuf.get_serialized_file_descriptor_set.ut.my_message_pb2 import TMyMessage


def test_get_serialized_file_descriptor_set():
    return base64.b64encode(get_serialized_file_descriptor_set(TMyMessage))
