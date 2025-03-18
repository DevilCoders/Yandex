
if False:  # pragma: nocover
    from ..http import HttpConnector  # noqa


class Endpoint:
    """Базовый класс для конечных точек."""

    url: str = None

    def __init__(self, http_connector: 'HttpConnector'):
        """
        :param http_connector:

        """
        self._connector = http_connector

    def _call(self, resource_name: str, data: dict = None, *, method: str = None) -> dict:
        url = self.url + resource_name

        if method is None:
            method = 'post' if data else 'get'

        return self._connector.request(url, data, method=method)


class EndpointSigning(Endpoint):
    """Объединяет конечные точки REST API DSS, связанные с подписями."""

    url = '/SignServer/rest/api/'


class EndpointSts(Endpoint):
    """Объединяет конечные точки Службы маркеров безопасности - Security Token Service (STS)."""

    url = '/STS/'


class EndpointUsers(Endpoint):
    """Объединяет конечные точки REST API DSS, связанные с управлением пользователями."""

    url = EndpointSts.url + 'ums/'
