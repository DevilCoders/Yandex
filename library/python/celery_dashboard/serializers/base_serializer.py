from abc import ABC, abstractmethod
from typing import Any, AnyStr


class SerializationError(BaseException):
    pass


class SerializerInitError(BaseException):
    pass


class BaseSerializer(ABC):
    @staticmethod
    @abstractmethod
    def serialize(obj: Any) -> AnyStr:
        pass

    @staticmethod
    @abstractmethod
    def deserialize(obj: bytes) -> Any:
        pass
