import base64
import json
import zlib

import six

from library.python.protobuf.get_serialized_file_descriptor_set import get_serialized_file_descriptor_set


def yql_proto_field(message_type, lists_optional=False, enum_mode="number"):
    if enum_mode not in ("number", "name", "full_name"):
        raise ValueError("unknown enum mode", enum_mode)

    return json.dumps({
        "name": message_type.DESCRIPTOR.full_name,
        "meta": six.ensure_text(base64.b64encode(zlib.compress(get_serialized_file_descriptor_set(message_type), 9))),
        "format": "protobin",
        "skip": 0,
        "lists": {
            "optional": lists_optional
        },
        "view": {
            "enum": enum_mode,
            "recursion": "fail"
        }
    })
