import mock
import pytest
from tornado.concurrent import Future
from tornado.httputil import HTTPServerRequest, HTTPHeaders

from yc_auth.exceptions import AuthFailureError
from yc_auth_tornado.authentication import TokenAuth


class Connection:
    class context:
        remote_ip = '1:2:3::4'


class Request(HTTPServerRequest):
    def __init__(self, *args, **kwargs):
        kwargs['connection'] = Connection()
        super(Request, self).__init__(*args, **kwargs)


@pytest.mark.gen_test
def test_token_auth_success(encoded_token):
    result = Future()
    result.set_result(True)
    gauthling_client = mock.MagicMock()
    gauthling_client.auth.return_value = result

    access_service_client = None

    headers = HTTPHeaders({"X-YaCloud-SubjectToken": encoded_token})
    request = Request("GET", "cloud.yandex/api", headers=headers)

    with pytest.raises(ValueError) as exc_info:
        auth_method = TokenAuth(lambda: gauthling_client, lambda: access_service_client)
        auth_ctx = yield auth_method.authenticate_tornado_request(request)

    assert exc_info.value.message == "Gauthling is no longer supported."


@pytest.mark.gen_test
def test_token_auth_failure(encoded_token):
    result = Future()
    result.set_result(False)
    gauthling_client = mock.MagicMock()
    gauthling_client.auth.return_value = result

    access_service_client = None

    headers = HTTPHeaders({"X-YaCloud-SubjectToken": encoded_token})
    request = Request("GET", "cloud.yandex/api", headers=headers)

    with pytest.raises(ValueError) as exc_info:
        auth_method = TokenAuth(lambda: gauthling_client, lambda: access_service_client)
        with pytest.raises(AuthFailureError) as exc_info:
            yield auth_method.authenticate_tornado_request(request)

    assert exc_info.value.message == "Gauthling is no longer supported."


@pytest.mark.gen_test
def test_token_auth_missing_token_header():
    result = Future()
    result.set_result(True)
    gauthling_client = mock.MagicMock()
    gauthling_client.auth.return_value = result

    request = Request("GET", "cloud.yandex/api")

    access_service_client = None

    with pytest.raises(ValueError) as exc_info:
        auth_method = TokenAuth(lambda: gauthling_client, lambda: access_service_client)
        with pytest.raises(AuthFailureError) as exc_info:
            yield auth_method.authenticate_tornado_request(request)

    assert exc_info.value.message == "Gauthling is no longer supported."
