from base64 import b64encode
from copy import deepcopy

from .exceptions import DssClientException
from .endpoints.base import EndpointSts

if False:  # pragma: nocover
    from .http import HttpConnector  # noqa


class Auth:
    """Базовый класс авторизации."""

    connector: 'HttpConnector' = None
    """Связанный объект HTTP-клиента. Проставляется экземпляру в ходе инициализации HTTP-клиента."""

    def get_auth_header(self, *, renew: bool = False) -> str:
        """Возвращает строку авторизации токен доступа.

        :param  renew: Следует ли вернуть обновлённый заголовок.

        """
        raise NotImplementedError  # pragma: nocover


class Oauth2Auth(Auth):
    """База для авторизации при помощи Oauth2."""

    def __init__(self, *, access_token: str = None):
        """
        :param access_token: Токен доступа к ресурсу.

        """
        self.access_token = access_token

    def get_auth_header(self, *, renew: bool = False) -> str:

        if not self.access_token:
            return ''

        return f'Bearer {self.access_token}'


class BasicAuth(Auth):
    """Авторизация типа Basic."""

    def __init__(self, *, username: str, password: str):
        self.username = username
        self.password = password

    def get_auth_header(self, renew=False):
        encoded = b64encode(':'.join((self.username, self.password)).encode('latin1'))
        return f"Basic {encoded.decode('latin1')}"


class OwnerAuth(Oauth2Auth):
    """Авторизация по реквизитам владельца ресурса.

    Использует сценарий OAuth2 - Resource Owner Password Credentials Grant

    https://tools.ietf.org/html/rfc6749#section-4.3

    """
    endpoint_access_token: str = EndpointSts.url + 'oauth/token'

    def __init__(
        self,
        *,
        username: str,
        password: str,
        client_id: str = None,
        client_secret: str = None,
        access_token: str = None
    ):
        super(OwnerAuth, self).__init__(access_token=access_token)

        self.client_id = client_id or 'cryptopro.cloud.csp'  # Встроенный клиент.
        self.client_secret = client_secret or ''  # У встроенного клиента отсутствует секрет.
        self.username = username
        self.password = password

    def get_auth_header(self, renew: bool = False) -> str:

        if renew:
            connector = deepcopy(self.connector)
            connector.auth = BasicAuth(username=self.client_id, password=self.client_secret)

            response = connector.request(self.endpoint_access_token, {
                'grant_type': 'password',
                'username': self.username,
                'password': self.password,
                'resource': 'urn:cryptopro:dss:signserver:SignServer',

            }, method='post', result=False)

            self.access_token = response['access_token']

        return super(OwnerAuth, self).get_auth_header(renew=renew)


class StaticAuth(Oauth2Auth):
    """Статичная авторизация по ранее полученному токену доступа.

    Не обновляет токен автоматически. Удобна для отладки и консольного агента.

    """
    def get_auth_header(self, renew: bool = False) -> str:

        if renew:
            raise DssClientException('Your static token is invalid and StaticAuth is unable to renew access token.')

        return super(StaticAuth, self).get_auth_header()
