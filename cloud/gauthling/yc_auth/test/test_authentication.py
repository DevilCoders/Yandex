from collections import defaultdict

import mock
import pytest
from werkzeug.datastructures import Headers

from yc_auth.authentication import TokenAuth
from yc_auth.exceptions import AuthFailureError, TooManyRequestsError


class Request:
    def __init__(self, method, path, params=None, headers=None, body=None):
        self.method = method
        self.data = body
        self.path = path
        self.args = params if params else {}
        self.headers = Headers(headers)
        self.access_route = ['1.2.3.4']


def test_token_auth_success(encoded_token):
    gauthling_client = mock.MagicMock()
    gauthling_client.auth.return_value = True

    access_service_client = None

    headers = {"X-YaCloud-SubjectToken": encoded_token}
    request = Request("GET", "cloud.yandex/api", headers=headers)

    with pytest.raises(ValueError) as exc_info:
        auth_method = TokenAuth(lambda: gauthling_client, lambda: access_service_client)
        auth_method.authenticate_flask_request(request)

    assert exc_info.value.message == "Gauthling is no longer supported."


def test_token_auth_failure(encoded_token):
    gauthling_client = mock.MagicMock()
    gauthling_client.auth.return_value = False

    access_service_client = None

    headers = {"X-YaCloud-SubjectToken": encoded_token}
    request = Request("GET", "cloud.yandex/api", headers=headers)

    with pytest.raises(ValueError) as exc_info:
        auth_method = TokenAuth(lambda: gauthling_client, lambda: access_service_client)
        auth_method.authenticate_flask_request(request)

    assert exc_info.value.message == "Gauthling is no longer supported."


def test_token_auth_missing_token_header():
    gauthling_client = mock.MagicMock()
    gauthling_client.auth.return_value = True

    access_service_client = None

    request = Request("GET", "cloud.yandex/api")

    with pytest.raises(ValueError) as exc_info:
        auth_method = TokenAuth(lambda: gauthling_client, lambda: access_service_client)
        with pytest.raises(AuthFailureError) as exc_info:
            auth_method.authenticate_flask_request(request)

    assert exc_info.value.message == "Gauthling is no longer supported."


def test_token_auth_multiple_token_headers(encoded_token):
    gauthling_client = mock.MagicMock()
    gauthling_client.auth.return_value = True

    access_service_client = None

    headers = Headers()
    headers.extend([("X-YaCloud-SubjectToken", encoded_token), ("X-YaCloud-SubjectToken", "invalid_token")])
    request = Request("GET", "cloud.yandex/api", headers=headers)

    with pytest.raises(ValueError) as exc_info:
        auth_method = TokenAuth(lambda: gauthling_client, lambda: access_service_client)
        auth_method.authenticate_flask_request(request)

    assert exc_info.value.message == "Gauthling is no longer supported."


def test_limits(encoded_token):
    gauthling_client = mock.MagicMock()
    gauthling_client.auth.return_value = True

    access_service_client = None

    headers = {"X-YaCloud-SubjectToken": encoded_token}
    request = Request("GET", "cloud.yandex/api", headers=headers)

    cache = defaultdict(int)
    def check_limits(auth_ctx):
        cache[auth_ctx.user.id] += 1
        return cache[auth_ctx.user.id] <= 5

    with pytest.raises(ValueError) as exc_info:
        auth_method = TokenAuth(lambda: gauthling_client, lambda: access_service_client, check_limits=check_limits)
        for _ in range(5):
            auth_method.authenticate_flask_request(request)

        assert exc_info.value.message == "Gauthling is no longer supported."

        with pytest.raises(TooManyRequestsError):
            auth_method.authenticate_flask_request(request)
