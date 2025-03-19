from __future__ import unicode_literals

import functools

import grpc
from grpc import StatusCode


class ErrorCodes:
    AuthFailure = "AuthFailure"
    IncompleteSignature = "IncompleteSignature"
    SignatureExpired = "SignatureExpired"
    SignatureDoesNotMatch = "SignatureDoesNotMatch"
    UnauthorizedOperation = "UnauthorizedOperation"
    TooManyRequests = "TooManyRequests"
    InternalError = "InternalError"
    ConnectionError = "ConnectionError"


class ErrorMessages:
    InvalidToken = "Invalid token."
    Unauthenticated = "Unauthenticated."


class YcAuthError(Exception):
    """Base class for authentication/authorization errors"""
    def __init__(self, http_code, code, message):
        super(YcAuthError, self).__init__(message)
        self.code = code
        self.message = message
        self.http_code = http_code
        self.internal = 500 <= http_code < 600


class YcAuthClientError(YcAuthError):
    """Base class for Client Errors"""
    def __init__(self, http_code, code, message):
        super(YcAuthClientError, self).__init__(http_code, code, message)


class AuthFailureError(YcAuthClientError):
    """Raised when credentials provided by client could not be validated"""
    def __init__(self, message):
        super(AuthFailureError, self).__init__(401, ErrorCodes.AuthFailure, message)


class IncompleteSignatureError(YcAuthClientError):
    """Raised when the request signature is absent or does not conform to YC standards"""
    def __init__(self):
        message = "The request signature is absent or does not conform to YC standards"
        super(IncompleteSignatureError, self).__init__(400, ErrorCodes.IncompleteSignature, message)


class SignatureExpiredError(YcAuthClientError):
    """Raised when signed timestamp is older then allowed age"""
    def __init__(self):
        message = "The request signature are older than allowed"
        super(SignatureExpiredError, self).__init__(400, ErrorCodes.SignatureExpired, message)


class SignatureDoesNotMatchError(YcAuthClientError):
    """Raised when request signature provided by client does not match
    with signature calculated on the server"""
    def __init__(self):
        message = "The request signature we calculated does not match the signature you provided. " \
                  "Check your secret key and signing method."
        super(SignatureDoesNotMatchError, self).__init__(401, ErrorCodes.SignatureDoesNotMatch, message)


class UnauthorizedOperationError(YcAuthClientError):
    """Raised when client does not have required permissions to carry out the request"""
    def __init__(self):
        message = "You are not authorized for this operation."
        super(UnauthorizedOperationError, self).__init__(403, ErrorCodes.UnauthorizedOperation, message)


class TooManyRequestsError(YcAuthClientError):
    """Raised when a service-specific limit on the number of requests is exceeded"""
    def __init__(self):
        message = "Request quota exceeded."
        super(TooManyRequestsError, self).__init__(429, ErrorCodes.TooManyRequests, message)


class ConnectionError(YcAuthError):
    """Raised when connection failure happened"""
    def __init__(self):
        message = "Connection error."
        super(ConnectionError, self).__init__(500, ErrorCodes.ConnectionError, message)


class InternalServerError(YcAuthError):
    """Raised when internal error agent happened"""
    def __init__(self):
        message = "Internal error."
        super(InternalServerError, self).__init__(500, ErrorCodes.InternalError, message)


class LogicalError(Exception):
    """Raised from code that should never be reached"""
    def __init__(self, *args):
        if args:
            message, args = args[0], args[1:]
            if args:
                message = message.format(*args)
        else:
            message = "Logical error."
        super(LogicalError, self).__init__(message)


def convert_to_yc_auth_error(grpc_error):
    error_status_code = grpc_error.code()
    # Signature mismatch or unsupported algorithm or key not found in SCMS or SCMS internal
    # grpc_error or no key for current day.
    if error_status_code == StatusCode.UNAUTHENTICATED:
        return AuthFailureError(ErrorMessages.Unauthenticated)
    # SCMS blacklist throttling check.
    elif error_status_code == StatusCode.RESOURCE_EXHAUSTED:
        return TooManyRequestsError()
    # Invalid passed IP address or invalid passed date.
    elif error_status_code == StatusCode.INVALID_ARGUMENT:
        return IncompleteSignatureError()
    # Internal server error.
    elif error_status_code == StatusCode.INTERNAL:
        return InternalServerError()
    elif error_status_code == StatusCode.UNAVAILABLE:
        return ConnectionError()
    return LogicalError("Unexpected grpc error: %s" % grpc_error)


def handle_grpc_error(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except grpc.RpcError as e:
            raise convert_to_yc_auth_error(e)

    return wrapper
