# flake8: noqa

from .base_serializer import BaseSerializer, SerializerInitError
from .json_serializer import JsonSerializer
from .msgpack_serializer import MsgPackSerializer
from .pickle_serializer import PickleSerializer


_serializers_list = {
    'json': JsonSerializer,
    'pickle': PickleSerializer,
    'yaml': None,
    'msgpack': MsgPackSerializer,
    'custom': None,
}


def get_serializer(serializer_name: str):
    serializer = _serializers_list.get(serializer_name)
    if serializer is None:
        raise SerializerInitError(f'Unknown serializer name {serializer_name}')

    return serializer
