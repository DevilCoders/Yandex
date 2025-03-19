import cgi
import functools
import os.path
import time
import urllib.parse
import sys
from typing import TypeVar, Type, Union

import requests
import typing
from requests.auth import AuthBase

import simplejson as json

from yc_requests.credentials import YandexCloudCredentials

from yc_common import logging
from yc_common import config
from yc_common import constants
from yc_common.context import get_context, update_context
from yc_common.exceptions import Error, ApiError, GrpcStatus
from yc_common.misc import drop_none, generate_id, ellipsis_string
from yc_common.models import ModelValidationError, Model, StringType, BooleanType, IntType
from yc_common.ya_clients import common


T = TypeVar("T")

log = logging.get_logger(__name__)

_MAIN_SCRIPT = os.path.basename(sys.argv[0])

_DEFAULT_USER_AGENT = " ".join(filter(bool, ["Yandex.Cloud", _MAIN_SCRIPT]))


class YcClientError(Error):
    def __init__(self, *args, request_uuid=None):
        super().__init__(*args)
        self.request_uuid = request_uuid

    def __str__(self):
        result = super().__str__()

        # Note: Check for context content to avoid information duplication in log messages
        #       First value comes from logging context, and the second one from this method.
        if self.request_uuid is not None and not get_context().get("request_uuid"):
            result += " (ruid={})".format(self.request_uuid)

        return result


class YcClientConnectionError(YcClientError):
    pass


class YcClientTimeoutError(YcClientError):
    pass


class YcClientProtocolError(YcClientError):
    pass


class YcClientInternalError(YcClientError):
    pass


class YcClientAuthorizationError(YcClientError):
    pass


class YcClientRequestValidationError(YcClientError):
    pass


class _ApiErrorModel(Model):
    status = IntType()
    code = StringType(required=True)
    message = StringType(required=True)
    internal = BooleanType(default=False)
    details = StringType()


# FIXME: Automatically retry idempotent methods
class ApiClient:
    API_CLIENT_METHODS = {"get", "put", "post", "patch", "delete"}
    def __init__(
            self,
            url,
            timeout=30,
            internal=False,
            extra_headers=None,
            service_name=None,                          # deprecated
            chunk_size=constants.MEGABYTE,
            credentials: YandexCloudCredentials=None,   # deprecated
            auth: AuthBase=None,
            iam_token=None,
            retry_temporary_errors=None,
            user_agent=_DEFAULT_USER_AGENT,
    ):
        self.__url = url.rstrip("/")
        self.__timeout = timeout
        self.__internal = internal
        self.__extra_headers = extra_headers or {}
        self.__auth = auth
        self.__chunk_size = chunk_size
        self.__json_requests = True
        self.__retry_temporary_errors = retry_temporary_errors
        self.__user_agent = user_agent

        if iam_token is not None:
            self.__iam_token = iam_token
        elif credentials is not None:
            self.__iam_token = credentials.token
        else:
            self.__iam_token = None

    def _set_json_requests(self, value):
        self.__json_requests = value

    def set_iam_token(self, iam_token):
        self.__iam_token = iam_token

    @property
    def chunk_size(self):
        return self.__chunk_size

    def get(self, path, params=None, model: Type[T]=None, timeout=None, stream=False,
            retry_temporary_errors=None, extra_headers=None, mask_fields=None) -> Union[T, dict]:
        return self.call("GET", path, params=params, model=model, timeout=timeout, stream=stream,
                         retry_temporary_errors=retry_temporary_errors, extra_headers=extra_headers, mask_fields=mask_fields)

    def put(self, path, request, params=None, files=None, model: Type[T]=None, timeout=None,
            retry_temporary_errors=None, extra_headers=None, mask_fields=None) -> Union[T, dict]:
        return self.call("PUT", path, params=params, files=files, request=request, model=model, timeout=timeout,
                         retry_temporary_errors=retry_temporary_errors, extra_headers=extra_headers,
                         mask_fields=mask_fields)

    def post(self, path, request, params=None, files=None, model: Type[T]=None, timeout=None,
             retry_temporary_errors=None, extra_headers=None, mask_fields=None) -> Union[T, dict]:
        return self.call("POST", path, params=params, files=files, request=request, model=model, timeout=timeout,
                         retry_temporary_errors=retry_temporary_errors, extra_headers=extra_headers,
                         mask_fields=mask_fields)

    def patch(self, path, request, params=None, files=None, model: Type[T]=None, timeout=None,
              retry_temporary_errors=None, extra_headers=None, mask_fields=None) -> Union[T, dict]:
        return self.call("PATCH", path, params=params, files=files, request=request, model=model, timeout=timeout,
                         retry_temporary_errors=retry_temporary_errors, extra_headers=extra_headers,
                         mask_fields=mask_fields)

    def delete(self, path, request=None, params=None, model: Type[T]=None, timeout=None,
               retry_temporary_errors=None, extra_headers=None, mask_fields=None) -> Union[T, dict]:
        return self.call("DELETE", path, params=params, request=request, model=model, timeout=timeout,
                         retry_temporary_errors=retry_temporary_errors, extra_headers=extra_headers,
                         mask_fields=mask_fields)

    def call(self, method, path, params=None, files=None, request=None, model: Type[T]=None, timeout=None, stream=None,
             retry_temporary_errors=None, extra_headers=None, mask_fields=None) -> Union[T, dict]:
        url = self.__url + path
        request_uid = generate_id()
        request_id = get_context().get("request_id", request_uid)

        if retry_temporary_errors is None:
            retry_temporary_errors = self.__retry_temporary_errors

        # CLOUD-25173: added context to local variables for post-mortem analysis
        with update_context(request_id=request_id, request_uid=request_uid) as context:
            log_message = "API call: %s %s"
            log_args = (method, _format_url(url, params))
            if request is not None:
                request = _to_api_obj(request)
                log_message += " %s"
                log_args += (logging.mask_sensitive_fields(request, extra_fields=mask_fields),)

            if stream and model is not None:
                raise YcClientInternalError("Parameter 'model' is incompatible with 'stream' flag.")

            try_number = 1
            retry_timeout_time = time.monotonic() + 3 * (self.__timeout if timeout is None else timeout)

            while True:
                log.debug(log_message, *log_args)
                request_start_time = time.monotonic()

                try:
                    result, raw_result = self.__call(
                        method, url, params, files, request, model, timeout, stream, extra_headers
                    )
                except Exception as e:
                    request_time = time.monotonic() - request_start_time

                    if isinstance(e, ApiError):
                        (log.debug if e.persistent() else log.error)(
                            "[rt=%.2f] API call returned error: %s", request_time, e)
                        retryable = not e.persistent()
                        e.request_id = request_id
                    elif isinstance(e, YcClientError):
                        log.error("[rt=%.2f] API call failed with error: %s", request_time, e)
                        retryable = isinstance(e, (YcClientConnectionError, YcClientTimeoutError, YcClientInternalError))
                        e.request_uuid = request_uid
                    else:
                        log.error("[rt=%.2f] API call failed with error: %s", request_time, e)
                        retryable = False

                    if not retry_temporary_errors or not retryable:
                        raise

                    sleep_time = 0 if isinstance(e, YcClientTimeoutError) else 1

                    if time.monotonic() + sleep_time >= retry_timeout_time:
                        # Translate it to a generic error to eliminate a possible occasional cascading retry
                        raise YcClientError("{!r} error after {} tries.", str(e), try_number)

                    log.warning("Retrying the API request (#%s)...", try_number)
                    try_number += 1
                    time.sleep(sleep_time)
                else:
                    request_time = time.monotonic() - request_start_time

                    if isinstance(raw_result, dict):
                        if log.isEnabledFor(logging.DEBUG):
                            log_result = logging.mask_sensitive_fields(raw_result, extra_fields=mask_fields)
                            log_result = ellipsis_string(str(log_result), 10000)
                            log.debug("[rt=%.2f] API call result: %s.", request_time, log_result)
                    elif isinstance(raw_result, requests.Response):
                        log.debug("[rt=%.2f] API call has completed with %s status code.",
                                  request_time, raw_result.status_code)
                    else:
                        log.debug("[rt=%.2f] API call has completed.", request_time)

                    return result

    def __call(self, method, url, params=None, files=None, request=None, model=None,
               timeout=None, stream=None, extra_headers=None):
        context = get_context()
        try:
            request_id = context.request_id
            request_uid = context.request_uid
        except AttributeError:
            # CLOUD-25173: Post-mortem analyze will be very handy in this case
            if config.test_mode():
                log.error("Attempted to access to non-existing attribute. Making core dump for post-mortem analysis")
                import os
                os.abort()
            raise
        headers = {
            "User-Agent": self.__user_agent,
            "Accept": "application/json",

            "X-Request-ID": request_id,
            "X-Request-UID": request_uid,
        }
        headers.update(self.__extra_headers)
        if extra_headers is not None:
            headers.update(extra_headers)

        if self.__iam_token is not None:
            headers.setdefault("X-YaCloud-SubjectToken", self.__iam_token)

        if self.__json_requests:
            if request is None:
                data = None
            else:
                headers["Content-Type"] = "application/json"
                data = json.dumps(request)
        else:
            data = request

        try:
            def set_stream_flag(r, *args, **kwargs):
                r.stream = kwargs.get("stream", False)

            _args = (method, url)
            _kwargs = dict(
                params=params,
                files=files,
                headers=headers,
                data=data,
                auth=self.__auth,
                timeout=self.__timeout if timeout is None else timeout,
                stream=stream or False,
                verify=common.get_verify(),
                hooks={"response": set_stream_flag},
            )
            response = requests.request(
                *_args,
                **_kwargs
            )
        except requests.RequestException as e:
            raise (YcClientTimeoutError if isinstance(e, requests.Timeout) else YcClientConnectionError)(
                "Request to {} failed: {}", url, e)

        return self._parse_response(response=response, model=model)

    def _parse_response(self, response, model: Model = None):
        """This method may be overridden by subclasses to support non Yandex Cloud API."""

        ok_codes = (requests.codes.ok, requests.codes.created, requests.codes.accepted, requests.codes.no_content)

        if response.status_code not in ok_codes:
            self._parse_error(response)

        if response.status_code == requests.codes.no_content:
            return None, None

        if response.stream:
            return response.iter_content(chunk_size=self.chunk_size), response

        return self._parse_json_response(response, model=model)

    def _parse_error(self, response):
        error, raw_result = self._parse_json_response(response, model=_ApiErrorModel)

        if response.status_code == requests.codes.unauthorized:
            raise YcClientAuthorizationError(error.message)
        # FIXME: Which error codes we should also ignore?
        elif error.code == "RequestValidationError":
            raise YcClientRequestValidationError(error.details or error.message)
        else:
            raise ApiError(
                response.status_code if error.status is None else GrpcStatus(error.status),
                error.code,
                error.message,
                error.details,
                internal=self.__internal or error.internal,
            )

    def _parse_json_response(self, response, model: typing.Type[Model] = None):
        content_type, type_options = cgi.parse_header(response.headers.get("Content-Type", ""))

        if response.status_code == requests.codes.bad_gateway and content_type != "application/json":
            raise YcClientConnectionError(
                "Request to {} has failed: The server returned '{} {}' status code.",
                response.url, response.status_code, response.reason)

        try:
            if content_type != "application/json":
                raise Error("Invalid Content-Type: {!r}.", content_type)

            try:
                result = raw_result = response.json()
            except ValueError:
                raise Error("Malformed JSON response.")
        except Error as e:
            self._on_invalid_response(response, e)

        try:
            if model is not None:
                result = model.from_api(result, ignore_unknown=True)
        except ModelValidationError as e:
            error_description = "Got an unexpected response on [{}] request to {} {}".format(
                get_context().request_uid, response.request.method, response.url)
            log.error("%s: %s (%s)", error_description, result, e)
            raise YcClientProtocolError("{}: {}", error_description, e)

        return result, raw_result

    @staticmethod
    def _on_invalid_response(response, error):
        error_description = "Got an invalid reply with {} status for [{}] request to {}".format(
            response.status_code, get_context().request_uid, response.url)
        log.error("%s: %s", error_description, error)
        raise YcClientProtocolError("{}.", error_description)


def retry_temporary_api_errors(message, timeout, extended_exceptions=()):
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            timeout_time = time.monotonic() + timeout
            try_number = 1

            while True:
                try:
                    return func(*args, **kwargs)
                except Exception as e:
                    if not (
                        isinstance(e, (YcClientConnectionError, YcClientTimeoutError)) or
                        isinstance(e, extended_exceptions) or
                        isinstance(e, ApiError) and not e.persistent()
                    ):
                        raise

                    sleep_time = 0 if isinstance(e, YcClientTimeoutError) else 1
                    if time.monotonic() + sleep_time >= timeout_time:
                        # Translate it to a generic error to eliminate a possible occasional cascading retry
                        raise YcClientError("{!r} error after {} tries.", str(e), try_number)

                    error = str(e)
                    if not error.endswith("."):
                        error += "."

                    log.warning("%s: '%s' Retrying temporary api errors (#%s)...", message, error, try_number)
                    try_number += 1
                    time.sleep(sleep_time)

        return wrapper

    return decorator


def _format_url(url, params=None):
    scheme, netloc, path, url_params, url_query, fragment = urllib.parse.urlparse(url)

    if params:
        query_params = urllib.parse.urlencode(drop_none(params), doseq=True)
        if query_params:
            if url_query:
                url_query += "&"
            url_query += query_params

    return urllib.parse.urlunparse((scheme, netloc, path, url_params, url_query, fragment))


def _to_api_obj(obj):
    if type(obj) is dict:
        return {key: _to_api_obj(value) for key, value in obj.items() if value is not None}
    elif type(obj) is list:
        return [_to_api_obj(value) for value in obj]
    elif isinstance(obj, Model):
        return obj.to_api(public=False)
    else:
        return obj
