import cgi
import functools
import os
import time
from typing import Tuple
import urllib.parse as urlparse
from itertools import islice
import requests

from oauthlib.oauth2 import WebApplicationClient
from oauthlib.oauth2.rfc6749.clients import base
from requests_oauthlib import OAuth2Session

from yc_common import config
from yc_common import logging
from yc_common.ya_clients import exception

LOG = logging.get_logger(__name__)


def chunks(l, n):
    for i in range(0, len(l), n):
        yield islice(l, i, i + n)


def prepare_yandex_headers(token, headers=None):
    headers = headers or dict()
    headers["Authorization"] = "OAuth {}".format(token)
    return headers


def raise_on_error(client_name: str, response: requests.Response, ok_statuses: Tuple[int]):
    status = response.status_code
    if status not in ok_statuses:
        data = {}
        try:
            data["error_message"] = response.json()
        except:
            data["error_message"] = response.text

        if status == 400:
            raise exception.BadRequest(client_name, data["error_message"])
        elif status == 401:
            raise exception.Unauthorized(client_name, data["error_message"])
        elif status == 404:
            raise exception.NotFound(client_name, data["error_message"])
        elif status in (500, 502):
            raise exception.ServerError(client_name, status, data["error_message"])
        else:
            raise exception.UnknownError(client_name, "Unknown error ({}): {}".format(status, data["error_message"]))


class YandexClient(WebApplicationClient):
    @property
    def token_types(self):
        types = super(YandexClient, self).token_types
        types.update({"Yandex": self._add_yandex_token})
        return types

    def _add_yandex_token(self, uri, http_method="GET", body=None,
                          headers=None, token_placement=None):
        if token_placement == base.AUTH_HEADER:
            headers = prepare_yandex_headers(self.access_token, headers)
        elif token_placement in (base.URI_QUERY, base.BODY):
            raise NotImplementedError("Unsupported token placement.")
        else:
            raise ValueError("Invalid token placement.")
        return uri, headers, body


class LazyResponse(object):
    def __init__(self, client, response):
        self._client = client
        self._response = response
        self._next = 0
        self._next_on_page = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self._next >= self._response["total"]:
            raise StopIteration
        else:
            if self._next_on_page >= self._response["limit"]:
                self._response = self._client.get(self._response["links"]["next"])
                self._next_on_page = 0

            cur = self._response["result"][self._next_on_page]
            self._next, self._next_on_page = self._next + 1, self._next_on_page + 1
            return cur


class LazyRequest(object):
    def __init__(self, client, urls):
        self._client = client
        self._urls = urls
        self._current_url = 0
        self._response = None

    def __iter__(self):
        while True:
            if not self._urls:
                raise StopIteration

            if not self._response:
                self._response = LazyResponse(self._client,
                                              self._client.get(self._urls[self._current_url]))

            try:
                yield next(self._response)
            except StopIteration:
                self._current_url += 1
                self._response = None
                if self._current_url == len(self._urls):
                    raise StopIteration


class IntranetClient(object):
    _client_name = "intranet"

    def __init__(self, base_url, auth_type=None, headers=None, timeout=5, certfile=None, keyfile=None, client_id=None,
                 token=None, ok_statuses=(200, 201)):
        self._session = None
        self._auth_type = auth_type
        self._timeout = timeout
        self._headers = headers
        if self._auth_type == "oauth":
            self._client_id = client_id
            self._token = token
        elif self._auth_type == "cert":
            self._cert = (certfile, keyfile)
        self._ok_statuses = ok_statuses

        url = urlparse.urlparse(base_url)
        if not url.scheme == "https":
            url = url._replace(scheme="https")
        self._base_url = url.geturl()

    @property
    def session(self):
        if not self._session:
            if self._auth_type == "oauth":
                token_param = {"access_token": self._token, "token_type": "yandex"}
                client = YandexClient(client_id=self._client_id, token=token_param)
                self._session = OAuth2Session(token=token_param, client=client)
            else:
                self._session = requests.session()
        return self._session

    def _compose_url(self, query):
        url = urlparse.urljoin(self._base_url, query)
        parsed_url = urlparse.urlparse(url)
        if not parsed_url.scheme == "https":
            parsed_url = parsed_url._replace(scheme="https")
        return parsed_url.geturl()

    def _prepare(self, data):
        req_kwargs = {"data": data, "timeout": self._timeout, "allow_redirects": False, "headers": self._headers,
                      "verify": get_verify()}
        if self._auth_type == "cert":
            req_kwargs["cert"] = self._cert
        return req_kwargs

    def _call(self, method, query, data=None):
        req_start = time.time()
        try:
            result = self.__call(method, query, data)
        except exception.ServerError as exc:
            LOG.error("[rt=%.2f] API call returned %r error: %s", time.time() - req_start, exc.code, exc)
            raise
        except exception.ClientError as exc:
            LOG.warning("[rt=%.2f] API call returned %r error: %s", time.time() - req_start, exc.code, exc)
            raise
        except requests.RequestException as exc:
            LOG.error("[rt=%.2f] API call error: %s", time.time() - req_start, exc)
            raise exception.RequestError(self._client_name, "Request failed: {}".format(exc))
        except Exception as exc:
            LOG.error("[rt=%.2f] API call failed with error: %s", time.time() - req_start, exc)
            if isinstance(exc, exception.UnknownError):
                raise
            else:
                raise exception.UnknownError(self._client_name, "Unknown exception: {}".format(exc))
        else:
            LOG.debug("[rt=%.2f] API call has completed.", time.time() - req_start)
            return result

    def __call(self, method, query, data):
        url = self._compose_url(query)
        req_args = (method, url)
        response = self.session.request(*req_args, **self._prepare(data))

        raise_on_error(self._client_name, response, self._ok_statuses)
        content_type, _ = cgi.parse_header(response.headers.get("Content-Type", ""))
        if content_type == "application/json":
            try:
                decoded_response = response.json()
            except ValueError as exc:
                raise exc
        else:
            decoded_response = response.text

        return decoded_response

    def get(self, query):
        return self._call("GET", query)

    def post(self, query, data):
        return self._call("POST", query, data)


def retry_server_errors(func):
    @functools.wraps(func)
    def decorator(*args, **kwargs):
        max_tries = 3
        sleep_time = 2

        curr_try = 0

        while curr_try < max_tries:
            try:
                return func(*args, **kwargs)
            except exception.ServerError as e:
                time.sleep(sleep_time)
                (LOG.warning if curr_try == 0 else LOG.error)(
                    "%s Retrying (%s of %s)...", e, curr_try + 1, max_tries)
                curr_try += 1

    return decorator


def get_verify():
    """Return path to verify sertificate or True.

    Check config for intranet_clients.ssl_verify value. If the config is not
    loaded yet and /etc/ssl/certs/ca-certificates.crt exists: return it.
    Otherwise return True
    """
    default = True
    if os.path.isfile("/etc/ssl/certs/ca-certificates.crt"):
        default = "/etc/ssl/certs/ca-certificates.crt"
    try:
        verify = config.get_value("intranet_clients.ssl_verify", default=default)
    except config.ConfigNotLoadedError:
        verify = default
    return verify
