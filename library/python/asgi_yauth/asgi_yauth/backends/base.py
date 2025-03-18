from typing import (
    Optional,
    Dict,
    AnyStr,
)
from enum import Enum
from ..user import (
    AnonymousYandexUser,
    YandexUser,
    BaseYandexUser,
)
from ..types import Scope, Headers
from ..settings import AsgiYauthConfig


class AnonymousReasons(Enum):
    no_backend = 'no_backend'


class BaseBackend:
    anonymous_yauser = AnonymousYandexUser
    yauser = YandexUser

    def __init__(self, config: AsgiYauthConfig) -> None:
        self.config = config

    async def authenticate(self, **kwargs) -> BaseYandexUser:
        raise NotImplementedError

    def anonymous(self, reason: Optional[AnyStr]) -> BaseYandexUser:
        return self.anonymous_yauser(backend=self, reason=reason)

    async def extract_params(self, scope: Scope, headers: Headers) -> Optional[Dict]:
        """
        Возвращает None, если аутентифицировать невозможно или
        словарь параметров для использования в self.authenticate
        """
        raise NotImplementedError

    @property
    def name(self) -> str:
        return self.__module__.split('.')[-1]

    def __repr__(self):
        return f'yauth backend "{self.name}"'
