from typing import Sequence
import functools
import time

import grpc
# There is this packet in arcadia
# from grpc_opentracing import open_tracing_client_interceptor
# from grpc_opentracing.grpcext import intercept_channel as ot_grpc_intercept_channel

from yc_common import logging
from yc_common.backoff import calc_exponential_backoff
from yc_common.context import get_context
from yc_common.exceptions import Error, LogicalError
from yc_common.fields import BooleanType, FloatType, IntType, ModelType, StringType
from yc_common.models import Model

log = logging.get_logger(__name__)

DEFAULT_TIMEOUT = 1
DEFAULT_RETRIES = 4
DEFAULT_BACKOFF_FACTOR = 0
READING_NON_FATAL_CODES = [
    grpc.StatusCode.UNKNOWN,
    grpc.StatusCode.UNAVAILABLE,
    grpc.StatusCode.ABORTED,
    grpc.StatusCode.DEADLINE_EXCEEDED,
]
WRITING_NON_FATAL_CODES = [
    grpc.StatusCode.UNKNOWN,
    grpc.StatusCode.UNAVAILABLE,
    grpc.StatusCode.ABORTED,
]


def _fix_continuation(continuation):
    @functools.wraps(continuation)
    def wrapper(*args, **kwargs):
        try:
            return continuation(*args, **kwargs)
        except grpc.RpcError as e:
            # TODO: continuation is supposed to return grpc.RpcError.
            # This bug was fixed in v1.18.0 (https://github.com/grpc/grpc/pull/17317)
            return e
    return wrapper


def _fix_interceptor(interceptor):
    intercept_unary_unary = interceptor.intercept_unary_unary
    interceptor.intercept_unary_unary = functools.wraps(intercept_unary_unary)(
        lambda continuation, details, request: intercept_unary_unary(
            _fix_continuation(continuation), details, request,
        )
    )
    return interceptor


def intercept_channel(channel, interceptor):
    return grpc.intercept_channel(channel, _fix_interceptor(interceptor))


class GrpcClientError(Error):
    def __init__(self, *args, code: grpc.StatusCode=None, details=None):
        message, args = args[0], args[1:]
        if code is not None:
            message += "; grpc_code={}, grpc_details={}".format(code.name, details)
        super().__init__(message, *args)
        self.code = code
        self.details = details


class GrpcClientNotFound(GrpcClientError):
    pass


class GrpcClientCredentials:
    def __init__(self,
                 enabled=False,
                 root_certs_file=None,
                 cert_file=None,
                 cert_private_key_file=None):
        self.enabled = enabled
        self.root_certs_file = root_certs_file
        self.cert_file = cert_file
        self.cert_private_key_file = cert_private_key_file

    def __eq__(self, other):
        return type(self) is type(other) and self.as_tuple() == other.as_tuple()

    def __hash__(self):
        return hash(self.as_tuple())

    def as_tuple(self):
        return self.enabled, self.root_certs_file, self.cert_file, self.cert_private_key_file

    def get_tls_channel_credentials(self):
        if not self.enabled:
            return grpc.ChannelCredentials(None)
        return grpc.ssl_channel_credentials(
            root_certificates=self._read_file_optional(self.root_certs_file),
            private_key=self._read_file_optional(self.cert_private_key_file),
            certificate_chain=self._read_file_optional(self.cert_file),
        )

    @classmethod
    def _read_file_optional(cls, filename=None):
        if filename is None:
            return None
        with open(filename, "rb") as f:
            return f.read()


class BaseGrpcClient:
    _channels = {}
    service_stub_cls = None

    def __init__(self,
                 endpoint,
                 timeout=None,
                 retries=None,
                 backoff_factor=DEFAULT_BACKOFF_FACTOR,
                 metadata=None,
                 tls=GrpcClientCredentials(),
                 tracer=None,
                 active_span_source=None):
        if (endpoint, tls) not in self._channels:
            self._channels[(endpoint, tls)] = grpc.secure_channel(endpoint, tls.get_tls_channel_credentials())
        self._channel = self._channels[(endpoint, tls)]

        self.timeout = timeout
        self.retries = retries
        self.backoff_factor = backoff_factor
        self.metadata = metadata
        self.tracer = tracer
        self.active_span_source = active_span_source
        self.reading_stub = self.service_stub_cls(self._wrap_channel(self._channel, READING_NON_FATAL_CODES))
        self.writing_stub = self.service_stub_cls(self._wrap_channel(self._channel, WRITING_NON_FATAL_CODES))

    def _wrap_channel(self, channel, non_fatal_codes):
        # Interceptor sequence before query is LIFO.
        # Interceptor sequence after query is FIFO.
        # In interceptors part of code before `continuation()` call is code before query. The same for after.
        # Exceptions which raises from interceptors will not raised from continuation() in next interceptors in stack,
        # but they will be returned from continuation() in _FailureOutcome object

        if self.retries is not None:
            channel = intercept_channel(channel, RetryInterceptor(
                retries=self.retries or self.retries,
                backoff_factor=self.backoff_factor,
                non_fatal_codes=non_fatal_codes,
            ))
        channel = intercept_channel(channel, ErrorLoggingInterceptor())
        if self.timeout is not None:
            channel = intercept_channel(channel, TimeoutInterceptor(timeout=self.timeout))
        channel = intercept_channel(channel, RequestIdInterceptor())
        channel = intercept_channel(channel, TimeLoggingInterceptor())

        if self.metadata is not None:
            channel = intercept_channel(channel, MetadataInterceptor(self.metadata))

        # if self.tracer is not None:
        #     channel = ot_grpc_intercept_channel(channel, open_tracing_client_interceptor(self.tracer,
        #                                                                                  self.active_span_source))

        return channel


def auth_metadata(iam_token):
    return [('authorization', 'Bearer ' + iam_token)]


class _ClientCallDetails(grpc.ClientCallDetails):
    pass


def _copy_details(details):
    new_details = _ClientCallDetails()
    new_details.method = details.method
    new_details.timeout = details.timeout
    new_details.metadata = details.metadata
    new_details.credentials = details.credentials
    return new_details


class TimeLoggingInterceptor(grpc.UnaryUnaryClientInterceptor):
    def intercept_unary_unary(self, continuation, details, request):
        log.info("gRPC request: %s...", details.method.split(".")[-1])

        start_time = time.monotonic()
        try:
            return continuation(details, request)
        finally:
            log.info("[rt=%.2f] gRPC request has completed.", time.monotonic() - start_time)


class RetryInterceptor(grpc.UnaryUnaryClientInterceptor):
    def __init__(self, retries, backoff_factor, non_fatal_codes):
        if retries is None or retries <= 1:
            raise LogicalError("Retries must be greater than 1, got {}", retries)
        if backoff_factor is None or backoff_factor < 0:
            raise LogicalError("Backoff factor must be greater or equal than 0, got {}", backoff_factor)

        self.retries = retries
        self.backoff_factor = backoff_factor
        self.non_fatal_codes = non_fatal_codes

    def get_backoff_time(self, number):
        backoff_value = calc_exponential_backoff(number, self.backoff_factor, rand=lambda: 1)
        return backoff_value

    def intercept_unary_unary(self, continuation, details, request):
        for number in range(self.retries):
            result = continuation(details, request)
            if not isinstance(result, grpc.RpcError):
                return result

            code = result.code()
            details = result.details()
            if code not in self.non_fatal_codes:
                return result

            backoff_time = self.get_backoff_time(number)
            log.info(
                "gRPC retryable error in %s times, retry with %.1f seconds; grpc_code=%s, grpc_details=%s.",
                number + 1, backoff_time, code, details,
            )
            if number + 1 < self.retries:
                time.sleep(backoff_time)
        return result


class ErrorLoggingInterceptor(grpc.UnaryUnaryClientInterceptor):
    def intercept_unary_unary(self, continuation, details, request):
        result = continuation(details, request)
        if not isinstance(result, grpc.RpcError):
            return result

        code = result.code()
        details = result.details()
        if code == grpc.StatusCode.NOT_FOUND:
            raise GrpcClientNotFound("gRPC request error", code=code, details=details)
        raise GrpcClientError("gRPC request error: " + details, code=code, details=details)


class RequestIdInterceptor(grpc.UnaryUnaryClientInterceptor):
    def intercept_unary_unary(self, continuation, details, request):
        request_id = get_context().get("request_id")
        if request_id is not None:
            details = _copy_details(details)
            details.metadata = details.metadata or []
            details.metadata.append(["x-request-id", request_id])
        return continuation(details, request)


class MetadataInterceptor(grpc.UnaryUnaryClientInterceptor):
    def __init__(self, metadata: Sequence):
        self.metadata = metadata

    def intercept_unary_unary(self, continuation, details, request):
        details = _copy_details(details)
        # metadata is actually a tuple at this point
        if details.metadata:
            details.metadata = list(details.metadata)
        else:
            details.metadata = []
        details.metadata.extend(self.metadata)

        return continuation(details, request)


class TimeoutInterceptor(grpc.UnaryUnaryClientInterceptor):
    def __init__(self, timeout):
        if timeout is None or timeout <= 0:
            raise LogicalError("Timeout must be greater than 0, got {}", timeout)

        self.timeout = timeout

    def intercept_unary_unary(self, continuation, details, request):
        details = _copy_details(details)
        details.timeout = details.timeout or self.timeout
        return continuation(details, request)


class TLSConfig(Model):
    enabled = BooleanType(required=True)
    # Client certificates are not required for ISO certification.
    cert_file = StringType()
    cert_private_key_file = StringType()
    root_certs_file = StringType()


class GrpcEndpointConfig(Model):
    url = StringType(required=True)
    timeout = FloatType(default=DEFAULT_TIMEOUT)
    retries = IntType(default=DEFAULT_RETRIES)
    backoff_factor = FloatType(default=DEFAULT_BACKOFF_FACTOR)
    tls = ModelType(TLSConfig, default=TLSConfig.new(enabled=False))


def get_client(client_cls, endpoint_config: GrpcEndpointConfig, metadata=None):
    return client_cls(
        endpoint=endpoint_config.url,
        timeout=endpoint_config.timeout,
        retries=endpoint_config.retries,
        backoff_factor=endpoint_config.backoff_factor,
        metadata=metadata,
        tls=GrpcClientCredentials(
            enabled=endpoint_config.tls.enabled,
            cert_file=endpoint_config.tls.cert_file,
            cert_private_key_file=endpoint_config.tls.cert_private_key_file,
            root_certs_file=endpoint_config.tls.root_certs_file,
        ),
    )
