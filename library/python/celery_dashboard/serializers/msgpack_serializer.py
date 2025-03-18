from typing import Any

import msgpack

from .base_serializer import BaseSerializer, SerializationError


class MsgPackSerializer(BaseSerializer):
    @staticmethod
    def serialize(obj: Any):
        try:
            msgpack_obj = msgpack.packb(obj, use_bin_type=True)
        except msgpack.PackException as e:
            raise SerializationError(e)

        return msgpack_obj

    @staticmethod
    def deserialize(msgpack_obj: bytes) -> Any:
        try:
            obj = msgpack.unpackb(msgpack_obj, raw=False)
        except msgpack.UnpackException as e:
            raise SerializationError(e)

        return obj
