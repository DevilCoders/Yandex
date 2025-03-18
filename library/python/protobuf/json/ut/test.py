from library.python.protobuf.json import (
    json2proto,
    proto2json,
)
from library.python.protobuf.json.ut.my_message_pb2 import TMyMessage


SRC = """{
    "inner_message": {
        "int32":10,
        "string":"string"
    },
    "float":1
}"""


def get_msg():
    msg = TMyMessage()
    msg.Float = 1.0
    msg.InnerMessage.Int32 = 10
    msg.InnerMessage.String = "string"
    return msg


def test_proto2json():
    config = proto2json.Proto2JsonConfig(format_output=True, field_name_mode=proto2json.FldNameMode.FieldNameSnakeCase)

    return proto2json.proto2json(get_msg(), config)


def test_proto2json_converter():
    config = proto2json.Proto2JsonConfig(format_output=True, field_name_mode=proto2json.FldNameMode.FieldNameSnakeCase)
    converter = proto2json.Proto2JsonConverter(TMyMessage, config)

    return converter.convert(get_msg())


def test_json2proto():
    msg = TMyMessage()

    config = json2proto.Json2ProtoConfig(field_name_mode=json2proto.FldNameMode.FieldNameSnakeCase)
    json2proto.json2proto(SRC, msg, config)
    assert get_msg() == msg


def test_proto_from_json():
    config = json2proto.Json2ProtoConfig(field_name_mode=json2proto.FldNameMode.FieldNameSnakeCase)
    assert get_msg() == json2proto.proto_from_json(SRC, TMyMessage, config)
