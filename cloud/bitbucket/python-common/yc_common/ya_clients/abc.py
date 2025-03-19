from urllib.parse import urlencode

from yc_common import config, logging
from yc_common.models import Model, StringType
from yc_common.ya_clients.common import IntranetClient, retry_server_errors


log = logging.get_logger(__name__)


class AbcEndpointConfig(Model):
    url = StringType(required=True)


class AbcOAuthConfig(Model):
    client_id = StringType(required=True)
    token = StringType(required=True)


class LazyResponse:
    def __init__(self, client, url, params=None, model=None):
        self._client = client
        self._url = url
        if params:
            self._url += '?' + urlencode(params, doseq=True)
        self.model = model

    @property
    def count(self):
        response = self._client.get(self._url)
        count = response["count"]
        return count

    @staticmethod
    def get_next_page_url(response):
        return response.get("next")

    @staticmethod
    def get_results(response):
        return response.get("results", [])

    def __iter__(self):
        response = self._client.get(self._url)
        results = self.get_results(response)
        while results:
            for item in results:
                yield item

            url = self.get_next_page_url(response)
            if not url:
                return

            response = self._client.get(url)
            results = self.get_results(response)


class AbcClient(IntranetClient):
    _client_name = "abc"

    def iter_services(self, params=None):
        services = LazyResponse(self, "services/", params=params)
        return services

    def iter_service_members(self, params=None):
        members = LazyResponse(self, "services/members/", params=params)
        return members

    def iter_roles(self, params=None):
        roles = LazyResponse(self, "roles/", params=params)
        return roles

    @retry_server_errors
    def get(self, query):
        return self._call("GET", query)


def get_abc_client(timeout=30) -> AbcClient:
    endpoint = config.get_value("endpoints.abc", model=AbcEndpointConfig,
                                default="https://abc-back.yandex-team.ru/api/v3/")
    auth = config.get_value("auth.abc.oauth", model=AbcOAuthConfig)
    return AbcClient(
        base_url=endpoint.url,
        auth_type="oauth",
        client_id=auth.client_id,
        token=auth.token,
        timeout=timeout
    )
