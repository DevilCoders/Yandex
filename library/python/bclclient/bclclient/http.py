import logging
from typing import List

import requests

from .auth import AuthArg, TvmAuth
from .exceptions import ConnectionError, ApiCallError
from .settings import DEFAULT_HOST, TVM_IDS, TVM_ID_TEST

if False:  # pragma: nocover
    from requests import Response


LOG = logging.getLogger(__name__)


class BclResponse:
    """Представляет ответ от BCL."""

    __slots__ = ['_data', 'response']

    def __init__(self, data: dict, response: 'Response'):
        self._data = data
        self.response = response

    @property
    def status(self) -> int:
        return self.response.status_code

    @property
    def data(self) -> dict:
        """Полезные данные из ответа."""
        return self._data['data']

    @property
    def errors(self) -> List[dict]:
        """Перечисление ошибок из ответа."""
        return self._data['errors']


class Connector:
    """Предоставляет интерфейс для запросов к API."""

    def __init__(self, auth: AuthArg, *, host: str = None, timeout: int = None):
        """
        :param auth: Данные для авторизации:
            * Объект TvmAuth
            * кортеж (id_приложения, секрет_приложения)

        :param host: Имя хоста, на котором находится BCL. Будет использован протокол HTTPS.
            Если не указан, будет использован хост по умолчанию (см. .settings.HOST_DEFAULT).

        :param timeout: Таймаут на подключение. По умолчанию: 10 сек.

        """
        host = host or DEFAULT_HOST

        self._timeout = timeout or 10
        self._verify = False

        url_base = host

        if not url_base.startswith('http'):
            url_base = f'https://{host}'

        self._url_base = f'{url_base}/api/'
        self._auth = TvmAuth.from_arg(auth, bcl_app_id=TVM_IDS.get(host, TVM_ID_TEST))
        self._service_alias = ''
        """Псевдоним сервиса. Используется для тестовой среды."""

    def request(self, *, url: str, data: dict = None, method: str = 'get', raise_on_error: bool = True):
        from . import VERSION_STR

        url = self._url_base + url

        is_get = method == 'get'
        if data and is_get:
            for key, val in data.items():
                if isinstance(val, (list, tuple, set)):
                    joined = '","'.join(map(str, val))
                    data[key] = f'["{joined}"]'

        method_ = getattr(requests, method)

        LOG.debug(f'URL: {url}')

        headers = {
            'User-agent': f'bclclient/{VERSION_STR}',
            'X-Ya-Service-Ticket': self._auth.get_ticket(),
        }

        service_alias = self._service_alias

        if service_alias:
            headers['X-Bcl-Service-Alias'] = service_alias

        try:
            response: 'Response' = method_(url, **{
                'params' if is_get else 'json': data,
            }, headers=headers, timeout=self._timeout, verify=self._verify)

        except requests.Timeout:
            raise ConnectionError('Request timed out.')

        except requests.ConnectionError:
            raise ConnectionError(f'Unable to connect to {url}.')

        if raise_on_error:
            try:
                response.raise_for_status()

            except requests.HTTPError:
                msg = response.text
                status_code = response.status_code
                LOG.debug(f'BCL API call error. Code {status_code}:\n{msg}')
                raise ApiCallError(msg, status_code)

        try:
            result = BclResponse(response.json(), response=response)

        except ValueError:
            # Здесь может быть HTML страница от nginx.
            raise ApiCallError(response.text, response.status_code)

        return result
