import aiohttp
import logging
import asyncio
import urllib.parse

from typing import Iterable, Optional, Any, NoReturn
from abc import ABC
from tenacity import (
    AsyncRetrying,
    RetryError,
    stop_after_attempt,
    wait_exponential,
    retry_if_exception_type,
    after_log,
    TryAgain,
)

from ..exceptions.base import (
    NoRetriesLeft,
    BadResponseStatus,
    AIOHTTPClientException,
    AuthKwargsMissing,
    InitClientError,
)
from ..auth_types import (
    AUTH_TYPES_MAP,
    TVM2,
    OAuth,
)

log = logging.getLogger(__name__)

ZORA_HOST = 'http://go.zora.yandex.net:1080'


class BaseClient(ABC):
    RETRY_CODES = {
        408,
        424,
        429,
        499,
        500,
        502,
        503,
        504,
    }

    RESPONSE_TYPE = 'json'

    AUTH_TYPES = (
        TVM2,
        OAuth,
    )

    USE_ZORA = False

    TIMEOUT = 5

    def __init__(
        self,
        host: str,
        timeout: int = None,
        retries: int = 3,
        wait_multiplier: float = 0.1,
        retry_codes: Optional[Iterable[int]] = None,
        auth_type: Optional[str] = None,
        log_headers_value: Optional[set] = None,
        **kwargs
    ):
        self.host = host
        self.timeout = timeout or self.TIMEOUT
        # так как ретраи учитывают все попытки
        # даже исходную - добавляем единицу к числу ретраев
        self.retries = retries + 1
        self.wait_multiplier = wait_multiplier
        self.retry_codes = retry_codes or self.RETRY_CODES
        self.kwargs = kwargs
        self.auth_headers = self.get_auth_headers(auth_type)
        self.log_headers_value = log_headers_value or set()

        if self.USE_ZORA and TVM2 not in self.AUTH_TYPES:
            raise InitClientError('zora requires tvm2 authentication')

    def get_auth_headers(self, use_type: Optional[str] = None) -> dict:
        auth_types = self.AUTH_TYPES
        if use_type:
            auth_types = (AUTH_TYPES_MAP[use_type],)

        if not auth_types:
            return {}

        for auth_type in auth_types:
            try:
                auth_instance = auth_type(**self.kwargs)
            except TypeError:
                continue

            return auth_instance.as_headers()

        message = (
            f'Required kwargs for any of {auth_types} are missing'
        )
        raise AuthKwargsMissing(message)

    def get_session(self, timeout: int = None) -> aiohttp.ClientSession:
        timeout = aiohttp.ClientTimeout(total=timeout or self.timeout)
        headers = self.get_headers()

        # do not send auth headers to external services
        # when using zora
        if not self.USE_ZORA:
            headers.update(self.auth_headers)

        return aiohttp.ClientSession(
            timeout=timeout,
            headers=headers,
        )

    def get_headers(self) -> dict:
        return {}

    async def parse_response(
            self,
            response: aiohttp.ClientResponse,
            response_type: str = None,
            **kwargs,
    ):
        if response.status in self.RETRY_CODES:
            raise TryAgain()
        try:
            response.raise_for_status()
        except aiohttp.ClientResponseError:
            message = (
                f'Got status: {response.status}, for request: '
                f'{response.method} {response.url}'
            )
            raise BadResponseStatus(message, status=response.status)

        response_type = response_type or self.RESPONSE_TYPE
        return await getattr(response, response_type)()

    def _prepare_params(self, params: Optional[dict]) -> Optional[dict]:
        return params

    def _prepare_headers_for_logging(self, headers):
        return {
            header: value if header in self.log_headers_value else '******'
            for header, value in headers.items()
        }

    def get_retries(
            self,
            path: str,
            method: str = 'get',
            params: Optional[dict] = None,
            json: Optional[dict] = None,
            data: Optional[Any] = None,
            headers: Optional[dict] = None,
            timeout: int = None,
            **kwargs
    ) -> int:
        return self.retries

    def get_proxy_params(self):
        if self.USE_ZORA:
            return [ZORA_HOST, self.get_auth_headers('tvm2')]

        return None, None

    async def _make_request(
        self,
        path: str,
        method: str = 'get',
        params: Optional[dict] = None,
        json: Optional[dict] = None,
        data: Optional[Any] = None,
        headers: Optional[dict] = None,
        timeout: int = None,
        response_type: str = None,
        **kwargs
    ):
        url = urllib.parse.urljoin(self.host, path)

        params = self._prepare_params(params)
        retries = self.get_retries(
            path,
            method,
            params,
            json,
            data,
            headers,
            timeout,
            **kwargs
        )
        proxy, proxy_headers = self.get_proxy_params()

        async with self.get_session(timeout=timeout) as session:
            try:
                async for attempt in AsyncRetrying(
                    stop=stop_after_attempt(retries),
                    retry=retry_if_exception_type(TryAgain),
                    wait=wait_exponential(multiplier=self.wait_multiplier),
                    after=after_log(log, logging.WARNING)
                ):
                    with attempt:
                        try:
                            async with getattr(session, method)(
                                url=url,
                                params=params,
                                headers=headers,
                                json=json,
                                data=data,
                                proxy=proxy,
                                proxy_headers=proxy_headers,
                                ssl=not self.USE_ZORA
                            ) as response:
                                log.info(
                                    '%s=%s %s (attempt: %s)',
                                    method.upper(),
                                    response.status,
                                    url,
                                    attempt.retry_state.attempt_number
                                )
                                return await self.parse_response(
                                    response=response,
                                    response_type=response_type,
                                    **kwargs
                                )
                        except (
                            aiohttp.ServerTimeoutError,
                            aiohttp.ServerDisconnectedError,
                            asyncio.TimeoutError,
                        ) as exc:
                            log.error(
                                '%s %s raised "%s". Retrying request.', method, url, str(exc))
                            raise TryAgain()
                        except aiohttp.ClientError as exc:
                            headers = getattr(exc, 'headers', None)
                            if headers is not None:
                                exc.headers = self._prepare_headers_for_logging(
                                    headers)
                            message = f'Got aiohttp exception: {repr(exc)}'
                            raise AIOHTTPClientException(
                                message, original_exception=exc)
            except RetryError:
                raise NoRetriesLeft()
