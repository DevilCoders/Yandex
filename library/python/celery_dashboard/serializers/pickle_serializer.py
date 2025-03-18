import pickle
from typing import Any
from io import BytesIO

from .base_serializer import BaseSerializer, SerializationError


class Unpickler(pickle.Unpickler):
    def find_class(self, module, name):
        return f'{module}.{name}'


class PickleSerializer(BaseSerializer):
    @staticmethod
    def serialize(obj: Any):
        try:
            pickle_obj = pickle.dumps(obj)
        except pickle.PickleError as e:
            raise SerializationError(e)

        return pickle_obj

    @staticmethod
    def deserialize(pickle_obj: bytes) -> Any:
        try:
            obj = Unpickler(BytesIO(pickle_obj)).load()
        except pickle.PickleError as e:
            raise SerializationError(e)

        return obj
