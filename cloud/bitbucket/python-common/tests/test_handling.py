from http import HTTPStatus
from unittest.mock import MagicMock
from unittest.mock import Mock

from yc_auth.exceptions import TooManyRequestsError

import yc_common.api.handling
from yc_common.api.handling import _ApiRequestBase
from yc_common.api.handling import format_api_error
from yc_common.api.handling import render_api_error
from yc_common.exceptions import ApiError
from yc_common.exceptions import GrpcStatus
from yc_common.exceptions import InternalServerError


class _TestRequest:
    method = "GET"


class _TestResponse:
    status_code = 200


class _TestMiddleware:
    @staticmethod
    def render_json_response(result, status=200):
        response = _TestResponse()
        response.status_code = status
        return response


class _TestError(ApiError):
    def __init__(self, internal, details="details"):
        super().__init__(HTTPStatus.NOT_FOUND, "code", "message", details=details, internal=internal)


def test_format_public_for_private():
    result = format_api_error(_TestError(internal=False))
    assert result == (
        {"code": "code", "message": "message", "details": "details", "internal": False, "status": GrpcStatus.INTERNAL},
        HTTPStatus.NOT_FOUND)


def test_format_private_for_private():
    result = format_api_error(_TestError(internal=True))
    assert result == (
    {"code": "code", "message": "message", "details": "details", "internal": True, "status": GrpcStatus.INTERNAL},
    HTTPStatus.NOT_FOUND)


def test_format_public_for_public():
    result = format_api_error(_TestError(internal=False), public=True)
    assert result == (
    {"code": "code", "message": "message", "details": "details", "internal": False, "status": GrpcStatus.INTERNAL},
    HTTPStatus.NOT_FOUND)


def test_format_private_for_public():
    result = format_api_error(_TestError(internal=True), public=True)

    error = InternalServerError()
    assert result == (
    {"code": error.code, "message": error.message, "internal": error.internal, "status": GrpcStatus.INTERNAL},
    error.http_code)


def test_api_request_counter_metric():
    yc_common.api.handling.api_request_counter = MagicMock()

    request = Mock(_TestRequest)
    request.method = "GET"
    middleware = Mock(_TestMiddleware)
    raw_path = "/test"
    r = _ApiRequestBase(request, middleware, render_api_error, raw_path)

    r.render_error(_TestError(internal=True))
    yc_common.api.handling.api_request_counter.labels.assert_called_once_with(404, "GET", "/test")

    yc_common.api.handling.api_request_counter.reset_mock()
    r.render_error(_TestError(internal=False))
    yc_common.api.handling.api_request_counter.labels.assert_called_once_with(404, "GET", "/test")

    yc_common.api.handling.api_request_counter.reset_mock()
    r.render_error(TooManyRequestsError())
    yc_common.api.handling.api_request_counter.labels.assert_called_once_with(500, "GET", "/test")

    yc_common.api.handling.api_request_counter.reset_mock()
    r.render_error(ValueError())
    yc_common.api.handling.api_request_counter.labels.assert_called_once_with(500, "GET", "/test")
