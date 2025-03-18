import json
from typing import Any

from .base_serializer import BaseSerializer, SerializationError


class JsonSerializer(BaseSerializer):
    @staticmethod
    def serialize(obj: Any):
        try:
            json_obj = json.dumps(obj)
        except Exception as e:
            raise SerializationError(e)

        return json_obj

    @staticmethod
    def deserialize(json_obj: bytes) -> Any:
        try:
            obj = json.loads(json_obj)
        except json.JSONDecodeError as e:
            raise SerializationError(e)

        return obj
