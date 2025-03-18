import requests
import logging
import os
import typing

from library.python import retry
from library.python.ok_client.structure import CreateApprovementRequest, GetApprovementResponse
from library.python.ok_client.exception import raise_unretriable_error

PROD_URL_BASE = "https://ok.yandex-team.ru"
TESTING_URL_BASE = "https://ok.test.yandex-team.ru"
PROD_API_URL = f"{PROD_URL_BASE}/api"
TESTING_API_URL = f"{TESTING_URL_BASE}/api"
DEFAULT_GET_RETRY_CONF = retry.DEFAULT_CONF.on(requests.HTTPError).waiting(delay=5, backoff=2, limit=3)
LOGGER = logging.getLogger(__name__)


class OkClient:
    """
    OK private API client

    https://wiki.yandex-team.ru/Intranet/OK/private-api/
    """

    DEFAULT_TIMEOUT_S = 60

    URL_PATH_APPROVEMENTS = "approvements"
    URL_PATH_APPROVEMENTS_SUSPEND = "suspend"
    URL_PATH_APPROVEMENTS_RESUME = "resume"
    URL_PATH_APPROVEMENTS_CLOSE = "close"

    UNRETRIABLE_STATUS_CODES = {
        requests.codes.bad_request,
        requests.codes.unauthorized,
        requests.codes.forbidden,
        requests.codes.not_allowed,
        requests.codes.not_implemented,
    }

    def __init__(self, token: str, base_url: str = PROD_API_URL, timeout: typing.Optional[int] = None):
        """
        :param token: OK API OAuth token
        :param base_url: (optional) OK API base URL
        :param timeout: (optional) request timeout, seconds
        """
        self._token = token
        self._base_url = base_url
        self._timeout = timeout or self.DEFAULT_TIMEOUT_S
        self._latest_response = None

    @property
    def base_url(self) -> str:
        return self._base_url

    @property
    def headers(self) -> typing.Dict[str, str]:
        return {
            "Authorization": f"OAuth {self._token}",
        }

    @property
    def latest_response(self) -> requests.Response:
        return self._latest_response

    def construct_url(self, *items) -> str:
        return os.path.join(self._base_url, *items, "")

    def _do_post(self, url: str, payload: dict = None) -> requests.Response:

        LOGGER.info("POST %s\nPayload: %s", url, payload)

        response = requests.post(
            url=url,
            json=payload,
            timeout=self._timeout,
            headers=self.headers,
            verify=False,
        )

        LOGGER.info(
            "Response received:\n"
            "  status: %s\n"
            "  content: %s\n",
            response.status_code,
            response.content,
        )

        self._latest_response = response

        self.check_response_status_code(response)

        return response

    def _do_get(self, url: str, payload: dict = None) -> requests.Response:

        LOGGER.info("GET %s\nPayload: %s", url, payload)

        response = requests.get(
            url=url,
            data=payload,
            timeout=self._timeout,
            headers=self.headers,
            verify=False,
        )

        LOGGER.info(
            "Response received:\n"
            "  status: %s\n"
            "  content: %s\n",
            response.status_code,
            response.content,
        )

        self._latest_response = response

        self.check_response_status_code(response)

        return response

    @retry.retry(conf=retry.DEFAULT_CONF.on(requests.HTTPError).waiting(delay=5, backoff=2, limit=3))
    def create_approvement(self, payload: CreateApprovementRequest) -> str:
        """
        Create and launch an approvement

        :return: Approvement UUID
        """
        url = self.construct_url(self.URL_PATH_APPROVEMENTS)
        response = self._do_post(url=url, payload=payload)
        return response.json()["uuid"]

    @retry.retry(conf=DEFAULT_GET_RETRY_CONF)
    def get_approvement(self, uuid: str) -> GetApprovementResponse:
        url = self.construct_url(self.URL_PATH_APPROVEMENTS, uuid)
        return self._do_get(url=url).json()

    @retry.retry(conf=DEFAULT_GET_RETRY_CONF)
    def suspend_approvement(self, uuid: str):
        url = self.construct_url(self.URL_PATH_APPROVEMENTS, uuid, self.URL_PATH_APPROVEMENTS_SUSPEND)
        self._do_post(url=url)

    @retry.retry(conf=DEFAULT_GET_RETRY_CONF)
    def resume_approvement(self, uuid: str):
        url = self.construct_url(self.URL_PATH_APPROVEMENTS, uuid, self.URL_PATH_APPROVEMENTS_RESUME)
        self._do_post(url=url)

    @retry.retry(conf=DEFAULT_GET_RETRY_CONF)
    def close_approvement(self, uuid: str):
        url = self.construct_url(self.URL_PATH_APPROVEMENTS, uuid, self.URL_PATH_APPROVEMENTS_CLOSE)
        self._do_post(url=url)

    def get_embed_url(self, uuid: str) -> str:
        return f"{os.path.join(PROD_URL_BASE, self.URL_PATH_APPROVEMENTS, uuid)}?_embedded=1"

    @classmethod
    def check_response_status_code(cls, response: requests.Response):

        if response.status_code not in cls.UNRETRIABLE_STATUS_CODES:
            response.raise_for_status()
            return

        error_data = None

        try:
            error_data = response.json()
        except (ValueError, TypeError):
            LOGGER.exception("Cannot retrieve error data from response object %s", response)

        logging.error("Got an error we do not really want to retry")

        raise_unretriable_error(f"{response.status_code}: {response.reason}", error_data)
