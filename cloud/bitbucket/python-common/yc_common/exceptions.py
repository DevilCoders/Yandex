"""Generic exception classes."""

import enum
import grpc

from yc_common.context import get_context


# https://github.com/googleapis/googleapis/blob/master/google/rpc/code.proto
class GrpcStatus(enum.IntEnum):

    def __new__(cls, grpc_code, http_code):
        value = grpc_code.value[0]
        obj = int.__new__(cls, value)
        obj._value_ = value
        obj.http_code = http_code
        return obj

    # Success
    OK = (grpc.StatusCode.OK, 200)

    # Operation cancelled, typically by caller
    CANCELLED = (grpc.StatusCode.CANCELLED, 499)

    # Errors raised by APIs that do not return enough error information
    UNKNOWN = (grpc.StatusCode.UNKNOWN, 500)

    # Arguments are problematic regardless of the state of the system
    INVALID_ARGUMENT = (grpc.StatusCode.INVALID_ARGUMENT, 400)

    # The deadline expired before the operation could complete
    DEADLINE_EXCEEDED = (grpc.StatusCode.DEADLINE_EXCEEDED, 504)

    # Some requested entity was not found.
    NOT_FOUND = (grpc.StatusCode.NOT_FOUND, 404)

    # The entity that a client attempted to create already exists.
    ALREADY_EXIST = (grpc.StatusCode.ALREADY_EXISTS, 409)

    # The caller does not have permission to execute the specified operation
    PERMISSION_DENIED = (grpc.StatusCode.PERMISSION_DENIED, 403)

    # The request does not have valid authentication credentials for the operation
    UNAUTHENTICATED = (grpc.StatusCode.UNAUTHENTICATED, 401)

    # Some resource has been exhausted
    RESOURCE_EXHAUSED = (grpc.StatusCode.RESOURCE_EXHAUSTED, 429)

    # The operation was aborted, typically due to a concurrency issue.
    # Client shoud retry at higher level
    ABORTED = (grpc.StatusCode.ABORTED, 409)

    # System is not in a state required for the operation's execution.
    # Client should not retry until the system state has been explicitly fixed.
    FAILED_PRECONDITION = (grpc.StatusCode.FAILED_PRECONDITION, 400)  # The operation was rejected because the system is not in a state required for operation execution

    # The operation was attempted past the valid range.
    OUT_OF_RANGE = (grpc.StatusCode.OUT_OF_RANGE, 400)

    # The operation is not implemented or is not supported / enabled in this service.
    UNIMPLEMENTED = (grpc.StatusCode.UNIMPLEMENTED, 501)

    # This means that some invariants expected by the underlying system have been broken.
    # This error code is reserved for serious errors.
    INTERNAL = (grpc.StatusCode.INTERNAL, 500)

    # The service is currently unavailable.  This is most likely a transient condition,
    # which can be corrected by retrying with a backoff.
    UNAVAILABLE = (grpc.StatusCode.UNAVAILABLE, 503)

    # Unrecoverable data loss or corruption.
    DATA_LOSS = (grpc.StatusCode.DATA_LOSS, 500)


class CommonErrorCodes:
    RequestValidationError = "RequestValidationError"
    InternalServerError = "InternalServerError"
    NotAuthorized = "NotAuthorized"
    Unauthenticated = "Unauthenticated"
    QuotaExceeded = "QuotaExceeded"
    ServiceUnavailable = "ServiceUnavailable"


class Error(Exception):
    """Base exception class with message formatting."""

    def __init__(self, *args):
        # Don't introduce any named parameters to be able to pass **kwargs to format() in the future
        message, args = args[0], args[1:]
        super().__init__(message.format(*args) if args else message)


class LogicalError(Exception):
    """Raised from code that should never be reached."""

    def __init__(self, *args):
        # Don't introduce any named parameters to be able to pass **kwargs to format() in the future

        if args:
            message, args = args[0], args[1:]
            if args:
                message = message.format(*args)
        else:
            message = "Logical error."

        super().__init__(message)


class ApiError(Error):
    """Base class for all API errors"""

    def __init__(self, http_code: int, code: str, message: str, details=None, internal=False, operation_id=None,
                 request_id=None):
        super().__init__(message if details is None else details)

        if isinstance(http_code, GrpcStatus):
            self.__grpc_code = int(http_code)
            self.__http_code = http_code.http_code
        else:
            self.__grpc_code = int(GrpcStatus.INTERNAL)
            self.__http_code = int(http_code)

        self.code = code
        self.message = message
        self.internal = internal
        self.details = details
        self.request_id = request_id
        self.operation_id = operation_id

    @property
    def http_code(self) -> int:
        return self.__http_code

    @property
    def grpc_code(self) -> int:
        return self.__grpc_code

    def __str__(self):
        result = "[{}] {}".format(self.code, super().__str__())

        # Note #1: Check for context content to avoid information duplication in log messages
        #       First value comes from logging context, and the second one from this method.
        # Note #2: To minimize error message we prefer operation id to request id
        if self.operation_id is not None and not get_context().get("operation_id"):
            result += " (oid={})".format(self.operation_id)
        elif self.request_id is not None and not get_context().get("request_id"):
            result += " (rid={})".format(self.request_id)

        return result

    def persistent(self):
        return self.http_code < 500 or self.http_code >= 600


class BadRequestError(ApiError):
    def __init__(self, code, message, details=None):
        super().__init__(GrpcStatus.INVALID_ARGUMENT, code, message, details)

    @classmethod
    def from_other(cls, error: ApiError):
        return cls(error.code, error.message, details=error.details)


class AuthenticationError(ApiError):
    def __init__(self, message, details=None):
        super().__init__(GrpcStatus.UNAUTHENTICATED, CommonErrorCodes.Unauthenticated, message, details)


class AuthorizationError(ApiError):
    def __init__(self, code, message, **kwargs):
        super().__init__(GrpcStatus.PERMISSION_DENIED, code, message, **kwargs)


class QuotaExceededError(ApiError):
    def __init__(self, message, **kwargs):
        super().__init__(GrpcStatus.RESOURCE_EXHAUSED, CommonErrorCodes.QuotaExceeded, message, **kwargs)


class ResourceNotFoundError(ApiError):
    def __init__(self, code, message, **kwargs):
        super().__init__(GrpcStatus.NOT_FOUND, code, message, **kwargs)


class ResourceAlreadyExistsError(ApiError):
    def __init__(self, code, message, **kwargs):
        super().__init__(GrpcStatus.ALREADY_EXIST, code, message, **kwargs)


class RequestConflictError(ApiError):
    def __init__(self, code, message, **kwargs):
        super().__init__(GrpcStatus.ABORTED, code, message, **kwargs)


class RequestAbortedError(ApiError):
    def __init__(self, code, message, **kwargs):
        super().__init__(GrpcStatus.ABORTED, code, message, **kwargs)


class PreconditionFailedError(ApiError):
    def __init__(self, code, message, **kwargs):
        super().__init__(GrpcStatus.FAILED_PRECONDITION, code, message, **kwargs)


class InternalServerError(ApiError):
    def __init__(self, code=CommonErrorCodes.InternalServerError, message="Internal server error has occurred.", **kwargs):
        super().__init__(GrpcStatus.INTERNAL, code, message, **kwargs)


class ServiceUnavailableError(ApiError):
    def __init__(self):
        super().__init__(GrpcStatus.UNAVAILABLE, CommonErrorCodes.ServiceUnavailable,
                         "Service temporary unavailable.")


# FIXME: Tune error code
class ApiRequestRateLimitExceededError(ApiError):
    def __init__(self, code, message, **kwargs):
        super().__init__(GrpcStatus.RESOURCE_EXHAUSED, code, message, **kwargs)


class RequestValidationError(BadRequestError):
    def __init__(self, message, *args):
        if args:
            message = message.format(*args)
        super().__init__(CommonErrorCodes.RequestValidationError, "Request validation error.",
                         "Request validation error: " + message)


class InvalidResourceIdError(RequestValidationError):
    def __init__(self):
        super().__init__("Invalid resource ID.")


class InvalidResourceNameError(RequestValidationError):
    def __init__(self):
        super().__init__("Invalid resource name.")
