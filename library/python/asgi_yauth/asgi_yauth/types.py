import typing

from pydantic import PyObject as BasePyObject
from tvmauth import BlackboxTvmId as BlackboxClientId

Scope = typing.MutableMapping[str, typing.Any]
Message = typing.MutableMapping[str, typing.Any]

Receive = typing.Callable[[], typing.Awaitable[Message]]
Send = typing.Callable[[Message], typing.Awaitable[None]]

ASGIApp = typing.Callable[[Scope, Receive, Send], typing.Awaitable[None]]


class BlackboxClient:
    BB_MAP = {
        'prod': BlackboxClientId.Prod,
        'mimino': BlackboxClientId.Mimino,
        'testing': BlackboxClientId.Test,
        'prod_yateam': BlackboxClientId.ProdYateam,
    }

    @classmethod
    def __get_validators__(cls):
        yield cls.validate

    @classmethod
    def validate(cls, value: str):
        if not isinstance(value, str):
            raise TypeError('string required')
        value = value.lower()
        if value not in cls.BB_MAP:
            raise TypeError(f'only {list(cls.BB_MAP.keys())} are allowed, got {value}')
        return cls.BB_MAP[value]


class PyBackendObject(BasePyObject):
    @classmethod
    def validate(cls, value: typing.Any) -> typing.Any:
        if isinstance(value, str):
            if '.' not in value:
                value = f'asgi_yauth.backends.{value}'
            value = f'{value}.Backend'
        return super().validate(value=value)


class Headers:
    """
    An immutable, case-insensitive multidict.
    """

    def __init__(
        self,
        scope: Scope = None,
    ) -> None:
        self._list = [
            (key.decode('utf-8'), value.decode('utf-8'))
            for key, value in scope.get("headers", [])
        ]
        self._dict = dict(self._list)

    @property
    def raw(self) -> typing.List[typing.Tuple[bytes, bytes]]:
        return list(self._list)

    def __getattr__(self, item):
        return getattr(self._dict, item)

    def get(self, key: str, default: typing.Any = None) -> typing.Any:
        try:
            return self[key]
        except KeyError:
            return default

    def _key_getter(self, key: str) -> typing.Tuple[typing.Any, bool]:
        upper_key = key.upper()
        lower_key = key.lower()
        for item in (upper_key, lower_key):
            if item in self._dict:
                return self._dict[item], True
        return None, False

    def __getitem__(self, key: str) -> str:
        value, found = self._key_getter(key)
        if found:
            return value
        raise KeyError(key)

    def __contains__(self, key: str) -> bool:
        _, found = self._key_getter(key)
        return found
