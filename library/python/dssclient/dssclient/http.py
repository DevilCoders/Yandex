import logging
from typing import Union

import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

from .auth import Auth, StaticAuth
from .exceptions import ApiCallError, ConnectionError
from .settings import DEFAULT_HOST

LOG = logging.getLogger(__name__)


class HttpConnector:
    """HTTP-клиент"""

    def __init__(self, auth: Union[Auth, str], host: str = None, timeout: int = None, retries: int = None):
        """
        :param auth: Объект, реализующий авторизацию (читай получение токена доступа).

        :param host: Имя хоста, на котором находится DSS. Будет использован протокол HTTPS.

        :param timeout: Таймаут на подключение. По умолчанию: 5 сек.

        :param retries: Максимальное число дополнительных попыток проведения запроса.
            По умолчанию: 7 штук.

        """
        if isinstance(auth, str):
            auth = StaticAuth(access_token=auth)

        auth.connector = self

        self.auth = auth
        self.host = host
        self.timeout = timeout
        self.retries = retries

    @classmethod
    def get_session(cls, retries: int = None) -> requests.Session:
        """Возвращает объект сессии requests.

        :param retries: Максимальное число дополнительных попыток проведения запроса.
            По умолчанию: 7 штук.

        """
        session = requests.Session()

        session.mount(
            'https://',
            HTTPAdapter(max_retries=Retry(
                total=retries or 7,
                backoff_factor=0.3
            ))
        )

        return session

    def _set_host(self, host: str):
        self._base_url = f'https://{host or DEFAULT_HOST}'

    host = property(None, _set_host)

    def _set_timeout(self, timeout: int):
        self._timeout = timeout or 5

    timeout = property(None, _set_timeout)

    def _set_retries(self, count: int):
        self._retries = count or 7

    retries = property(None, _set_retries)

    def request(self, url: str, data: dict = None, *, method: str = 'get', result: bool = True) -> dict:
        from . import version_str

        url = self._base_url + url
        timeout = self._timeout
        auth_count = [0]

        method_ = getattr(self.get_session(self._retries), method)

        # Не журналируем данные намеренно, чтобы не разглашать потенциально
        # важную информацию, например, авторизационную, а также пины.
        LOG.debug('URL: %s', url)

        def do_request(renew_auth: bool = False) -> requests.Response:

            headers = {
                'User-agent': f'dssclient/{version_str}',
            }

            auth_header = self.auth.get_auth_header(renew=renew_auth)

            if auth_header:
                headers['Authorization'] = auth_header

            try:
                resp = method_(url, **{
                    'headers': headers,
                    'json' if result else 'data': data,
                    'timeout': timeout,
                    'verify': False

                })

            except requests.ReadTimeout as e:
                raise ConnectionError('Request timed out.', culprit=e)

            except requests.ConnectionError as e:
                raise ConnectionError(f'Unable to connect to {url}: {e}.', culprit=e)

            try:
                resp.raise_for_status()

            except requests.HTTPError as e:

                if e.response.status_code == 401:
                    # Требуется [повторная] авторизация.
                    if auth_count[0] > 1:
                        # Рекурсия в ходе авторизации.
                        raise ConnectionError('Authorization failed.')

                    auth_count[0] += 1
                    resp = do_request(renew_auth=True)

                else:
                    msg = resp.content
                    LOG.error('API call error:\n%s', msg)
                    raise ApiCallError(msg)

            return resp

        response = do_request()

        try:
            if response.content:
                result = response.json()

            else:
                # В некоторых случаях API отвечает не JSON, а пустым документом.
                # Приводим к правильному виду.
                result = {}

        except ValueError:
            # Здесь может быть HTML страница от IIS.
            raise ApiCallError(response.content)

        return result
