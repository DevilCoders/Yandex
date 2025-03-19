import itertools
import logging
import time

import grpc
import grpc._channel

from yandex.cloud.priv.servicecontrol.v1 import access_service_pb2
from yandex.cloud.priv.servicecontrol.v1 import access_service_pb2_grpc

from yc_as_client import entities
from yc_as_client import exceptions


DEFAULT_CALL_TIMEOUT = 2
DEFAULT_MAX_RETRY_ATTEMPTS = 7
DEFAULT_INITIAL_BACKOFF = 0.1
DEFAULT_MAX_BACKOFF = 3
DEFAULT_BACKOFF_MULTIPLIER = 2

_RETRYABLE_STATUS_CODES = {
    grpc.StatusCode.ABORTED,
    grpc.StatusCode.CANCELLED,
    grpc.StatusCode.DEADLINE_EXCEEDED,
    grpc.StatusCode.UNAVAILABLE,
    grpc.StatusCode.INTERNAL,
    grpc.StatusCode.FAILED_PRECONDITION,
}


# this kind of logger name mangling is used in yc_common
__logger_name = __name__.replace("yc_", "yc.", 1)
log = logging.getLogger(__logger_name)


class YCAccessServiceRetryPolicy(object):
    def __init__(
        self,
        max_attemps=DEFAULT_MAX_RETRY_ATTEMPTS,
        initial_backoff=DEFAULT_INITIAL_BACKOFF,
        max_backoff=DEFAULT_MAX_BACKOFF,
        backoff_multiplier=DEFAULT_BACKOFF_MULTIPLIER,
    ):
        self.max_attempts = max_attemps
        self.initial_backoff = initial_backoff
        self.max_backoff = max_backoff
        self.backoff_multiplier = backoff_multiplier

    def gen_exponential_backoff(self):
        for i in itertools.count():
            yield min(self.max_backoff, self.initial_backoff * (2 ** i))

    def can_retry_status_code(self, status_code):
        return (status_code in _RETRYABLE_STATUS_CODES)


class YCAccessServiceClient(object):
    def __init__(
        self,
        channel,
        timeout=DEFAULT_CALL_TIMEOUT,
        retry_policy=None,  # type: Union[None, YCAccessServiceRetryPolicy]
    ):
        self._grpc_channel = channel
        self._grpc_stub = access_service_pb2_grpc.AccessServiceStub(self._grpc_channel)
        self._timeout = timeout
        self._retry_policy = retry_policy if retry_policy is not None else YCAccessServiceRetryPolicy()

        self.authenticate = self._futurize(self.authenticate)
        self.authorize = self._futurize(self.authorize)

    def authenticate(self, iam_token=None, iam_cookie=None, signature=None, api_key=None, request_id=None):
        _number_of_identity_params = sum(1 for x in (iam_token, iam_cookie, signature, api_key) if x is not None)
        if _number_of_identity_params != 1:
            raise ValueError(
                "Exactly one of `iam_token`, `iam_cookie`, `signature`, `api_key` must be specified."
            )

        if iam_token is not None:
            request = access_service_pb2.AuthenticateRequest(
                iam_token=iam_token,
            )
        elif iam_cookie is not None:
            request = access_service_pb2.AuthenticateRequest(
                iam_cookie=iam_cookie,
            )
        elif signature is not None:
            request = access_service_pb2.AuthenticateRequest(
                signature=signature._to_grpc_message(),
            )
        else:
            request = access_service_pb2.AuthenticateRequest(
                api_key=api_key,
            )

        return _Procedure(
            self._grpc_stub.Authenticate,
            request,
            request_id,
            lambda result: entities.Subject._from_grpc_message(result.subject),
            self._timeout,
            self._retry_policy,
        )

    def authorize(
        self,
        permission,
        resource_path,
        subject=None, iam_token=None, signature=None, api_key=None, request_id=None,
    ):
        _number_of_identity_params = sum(1 for x in (subject, iam_token, signature, api_key) if x is not None)
        if _number_of_identity_params != 1:
            raise ValueError(
                "Exactly one of `subject`, `iam_token`, `signature`, `api_key` must be specified."
            )

        if subject is not None:
            grpc_subject = subject._to_grpc_message()
            request = access_service_pb2.AuthorizeRequest(
                subject=grpc_subject,
            )
        elif iam_token is not None:
            request = access_service_pb2.AuthorizeRequest(
                iam_token=iam_token,
            )
        elif signature is not None:
            request = access_service_pb2.AuthorizeRequest(
                signature=signature._to_grpc_message(),
            )
        else:
            request = access_service_pb2.AuthorizeRequest(
                api_key=api_key,
            )

        request.permission = permission

        if isinstance(resource_path, entities.Resource):
            resource_path = [resource_path]
        elif resource_path is None or len(resource_path) == 0:
            raise ValueError("At least one resource is required.")
        request.resource_path.extend([r._to_grpc_message() for r in resource_path])

        return _Procedure(
            self._grpc_stub.Authorize,
            request,
            request_id,
            lambda result: entities.Subject._from_grpc_message(result.subject),
            self._timeout,
            self._retry_policy,
        )

    def _futurize(self, f):
        class _Wrapper(object):
            def __init__(self, f):
                self.__name__ = f.__name__
                self.__doc__ = f.__doc__

            def __call__(self, *args, **kwargs):
                procedure = f(*args, **kwargs)
                return procedure()

            def future(self, *args, **kwargs):
                procedure = f(*args, **kwargs)
                return procedure.future()

        return _Wrapper(f)


class _Procedure(object):
    def __init__(self, procedure, request, request_id, response_formatter, default_timeout, retry_policy):
        self._grpc_procedure = procedure
        self._grpc_request = request
        self._response_formatter = response_formatter
        self._default_timeout = default_timeout
        self._retry_policy = retry_policy
        self._request_id = request_id

    def __call__(self):
        metadata = []
        if self._request_id:
            metadata.append(("x-request-id", self._request_id))

        attempts_left = self._retry_policy.max_attempts
        for backoff in self._retry_policy.gen_exponential_backoff():
            start = time.monotonic()
            try:
                result = self._grpc_procedure(
                    self._grpc_request,
                    timeout=self._default_timeout,
                    metadata=metadata
                )
                return self._response_formatter(result)
            except grpc.RpcError as e:
                req_time = time.monotonic() - start

                if attempts_left >= 0 and self._retry_policy.can_retry_status_code(e.code()):
                    method = self._grpc_procedure._method
                    if isinstance(method, bytes):
                        method = method.decode()

                    log.warning(
                        "YCAccessService retryable request failed: \nreq_time=%.4f\nmethod=%s\nretry_attempts_left=%d"
                        "\nerror=%s", req_time, method, attempts_left, e
                    )
                    attempts_left -= 1
                    time.sleep(backoff)
                    continue
                else:
                    raise _encapsulate_exception(e)

    def future(self, **kwargs):
        if "timeout" not in kwargs:
            kwargs["timeout"] = self._default_timeout

        metadata = []
        if self._request_id:
            metadata.append(("x-request-id", self._request_id))

        return _Future(
            grpc_future=self._grpc_procedure.future(self._grpc_request, metadata=metadata, **kwargs),
            response_formatter=self._response_formatter,
        )


class _Future(grpc._channel._Rendezvous):
    def __init__(self, grpc_future, response_formatter):
        self._grpc_future = grpc_future
        self._response_formatter = response_formatter

    def result(self, *args, **kwargs):
        # NOTE: `result` and `__init__` both accept a `timeout` argument
        try:
            result = self._grpc_future.result(*args, **kwargs)
            return self._response_formatter(result)
        except grpc.RpcError as e:
            raise _encapsulate_exception(e)

    def __getattr__(self, name):
        """Catch-all method for delegating all unknown methods
        to the underlying GRPC future."""
        return getattr(self._grpc_future, name)


def _encapsulate_exception(exception):
    """Encapsulates a raw GRPC exception
    into a more specific Python exception class."""
    e_code = exception.code()
    return exceptions._grpc_code_to_exception_mapping[e_code](exception)
