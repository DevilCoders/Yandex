import functools
import logging
import time
import uuid

import grpc
from google.rpc import status_pb2
from schematics import Model
from schematics import types

DEFAULT_TIMEOUT = 1
DEFAULT_RETRIES = 4
DEFAULT_BACKOFF_FACTOR = 0
DEFAULT_RETRIES = 5
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

log = logging.getLogger(__name__)


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
                 options=None,
                 tracer=None,
                 active_span_source=None):
        if (endpoint, tls) not in self._channels:
            if options:
                options = [(k, v) for k, v in options.items()]
            self._channels[(endpoint, tls)] = grpc.secure_channel(
                endpoint, tls.get_tls_channel_credentials(), options=options
            )
        self._channel = self._channels[(endpoint, tls)]

        self.timeout = timeout
        if retries is None:
            retries = DEFAULT_RETRIES
        self.retries = retries
        self.backoff_factor = backoff_factor
        self.metadata = metadata
        self.tracer = tracer
        self.active_span_source = active_span_source
        self.reading_stub = self.service_stub_cls(self._wrap_channel(self._channel, READING_NON_FATAL_CODES, endpoint))
        self.writing_stub = self.service_stub_cls(self._wrap_channel(self._channel, WRITING_NON_FATAL_CODES, endpoint))

    def _wrap_channel(self, channel, non_fatal_codes, endpoint):
        # Interceptor sequence before query is LIFO.
        # Interceptor sequence after query is FIFO.
        # In interceptors part of code before `continuation()` call is code before query. The same for after.
        # Exceptions which raises from interceptors will not raised from continuation() in next interceptors in stack,
        # but they will be returned from continuation() in _FailureOutcome object

        if self.retries != 0:
            channel = intercept_channel(channel, RetryInterceptor(
                retries=self.retries,
                backoff_factor=self.backoff_factor,
                non_fatal_codes=non_fatal_codes,
            ))
        channel = intercept_channel(channel, ErrorLoggingInterceptor(endpoint))
        if self.timeout is not None:
            channel = intercept_channel(channel, TimeoutInterceptor(timeout=self.timeout))
        channel = intercept_channel(channel, RequestIdInterceptor())
        channel = intercept_channel(channel, TimeLoggingInterceptor())

        if self.metadata is not None:
            channel = intercept_channel(channel, MetadataInterceptor(self.metadata))

        return channel


def auth_metadata(iam_token):
    return [('authorization', 'Bearer ' + iam_token)]


def idempotency_key_metadata(idempotency_key):
    return [('idempotency-key', idempotency_key)]


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
        log.debug("gRPC request: %s...", details.method.split(".")[-1])

        start_time = time.time()
        try:
            return continuation(details, request)
        finally:
            log.debug("[rt=%.2f] gRPC request has completed.", time.time() - start_time)


class RetryInterceptor(grpc.UnaryUnaryClientInterceptor):
    def __init__(self, retries, backoff_factor, non_fatal_codes):
        if retries is None or retries <= 1:
            raise RuntimeError("Retries must be greater than 1, got {}", retries)
        if backoff_factor is None or backoff_factor < 0:
            raise RuntimeError("Backoff factor must be greater or equal than 0, got {}", backoff_factor)

        self.retries = retries
        self.backoff_factor = backoff_factor
        self.non_fatal_codes = non_fatal_codes

    def get_backoff_time(self, number):
        return self.backoff_factor * 2 ** number

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


def is_status_detail(x):
    if (hasattr(x, 'key') and (hasattr(x, 'value')) and x.key.startswith('grpc-status-details')):
        return True
    return False


def unpack_status_exception(result):
    if not isinstance(result, grpc.Call):
        return None

    if result.trailing_metadata() is None:
        return None

    trailing_metadata = [meta.value for meta in result.trailing_metadata() if is_status_detail(meta)]

    if len(trailing_metadata) == 1:
        [metadata] = trailing_metadata
        status = status_pb2.Status()
        status.MergeFromString(metadata)
        return status


class ErrorLoggingInterceptor(grpc.UnaryUnaryClientInterceptor):
    def __init__(self, endpoint):
        self._endpoint = endpoint

    def intercept_unary_unary(self, continuation, details, request):
        result = continuation(details, request)
        if not isinstance(result, grpc.RpcError):
            return result

        msg = "gRPC request {}{} error".format(self._endpoint, details.method)
        code = result.code()
        details = result.details()
        status = unpack_status_exception(result)

        raise RuntimeError(
            "msg: {msg}, code: {code}, details: {details}, status: {status}".format(msg=msg, code=code, details=details,
                                                                                    status=status))


class RequestIdInterceptor(grpc.UnaryUnaryClientInterceptor):
    def intercept_unary_unary(self, continuation, details, request):
        request_id = str(uuid.uuid4())
        details = _copy_details(details)
        details.metadata = details.metadata or []
        details.metadata.append(["x-request-id", request_id])
        return continuation(details, request)


class MetadataInterceptor(grpc.UnaryUnaryClientInterceptor):
    def __init__(self, metadata):
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
            raise RuntimeError("Timeout must be greater than 0, got {}", timeout)

        self.timeout = timeout

    def intercept_unary_unary(self, continuation, details, request):
        details = _copy_details(details)
        details.timeout = details.timeout or self.timeout
        return continuation(details, request)


class TLSConfig(Model):
    enabled = types.BooleanType(required=True)
    # Client certificates are not required for ISO certification.
    cert_file = types.StringType()
    cert_private_key_file = types.StringType()
    root_certs_file = types.StringType()


class GrpcEndpointConfig(Model):
    url = types.StringType(required=True)
    timeout = types.FloatType(default=DEFAULT_TIMEOUT)
    retries = types.IntType(default=DEFAULT_RETRIES)
    backoff_factor = types.FloatType(default=DEFAULT_BACKOFF_FACTOR)
    tls = types.ModelType(TLSConfig, default=TLSConfig({"enabled": False}))
    options = types.DictType(types.StringType, default=None)


def get_client(
    client_cls,
    endpoint_config,
    metadata=None,
    tracer=None,
):
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
        options=endpoint_config.options,
        tracer=tracer
    )
