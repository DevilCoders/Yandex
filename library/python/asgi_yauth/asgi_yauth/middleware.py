import logging
import functools

from .types import ASGIApp, Scope, Receive, Send, Headers
from .settings import AsgiYauthConfig
from .user import BaseYandexUser
from .backends.base import AnonymousReasons

log = logging.getLogger(__name__)


class YauthMiddleware:
    def __init__(self, app: ASGIApp, config: AsgiYauthConfig) -> None:
        self.app = app
        self.config = config
        self.anonymous_class = self.config.anonymous_user_class

    @property
    @functools.lru_cache()
    def backends(self):
        return [
            backend(config=self.config)
            for backend in self.config.backends
        ]

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:
        scope['user'] = await self.authenticate(scope=scope)
        await self.app(scope, receive, send)

    async def authenticate(self, scope: Scope) -> BaseYandexUser:
        headers = Headers(scope=scope)
        for backend in self.backends:
            params = await backend.extract_params(scope=scope, headers=headers)
            if params is not None:
                return await backend.authenticate(**params)

        log.debug('No applicable backend for this request')
        return self.anonymous_class(reason=AnonymousReasons.no_backend.value)


class YauthTestMiddleware(YauthMiddleware):
    async def authenticate(self, scope: Scope):
        backend = None
        backends = self.config.backends
        if backends:
            backend = backends[0](config=self.config)
        return self.config.test_user_class(
            backend=backend,
            **self.config.test_user_data
        )
