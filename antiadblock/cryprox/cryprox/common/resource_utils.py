from tornado.httpclient import AsyncHTTPClient, HTTPRequest
from tornado.gen import coroutine, Return

from antiadblock.cryprox.cryprox.config.service import FETCH_RESOURCE_WAIT_INITIAL, FETCH_RESOURCE_ATTEMPTS, FETCH_RESOURCE_WAIT_MULTIPLIER
from antiadblock.cryprox.cryprox.common.tools.tornado_retry_request import http_retry


@coroutine
def read_resource_from_url(url, headers=None, attempts=FETCH_RESOURCE_ATTEMPTS, initial_wait_time=FETCH_RESOURCE_WAIT_INITIAL, multiplier=FETCH_RESOURCE_WAIT_MULTIPLIER):
    """
    :param multiplier: wait time multiplier between retries
    :param initial_wait_time:
    :param attempts: how many attempts to fetch configs will be done
    :param url: url to read config from, duh
    :param headers: request headers
    :return: str with resource
    """

    http_client = AsyncHTTPClient(force_instance=True)
    headers = headers or {}
    request = HTTPRequest(url=url, headers=headers)
    response = yield http_retry(http_client, request, tries=attempts, delay=initial_wait_time, backoff=multiplier)
    raise Return(response.body)


def url_to_filename(filename):
    return filename.replace("\\", "_").replace("/", "_")
