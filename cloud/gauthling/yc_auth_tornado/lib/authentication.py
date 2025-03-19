import logging

import yc_as_client.exceptions
from tornado import gen
from yc_auth.authentication import (
    AuthContext,
    TokenAuth as _TokenAuth,
)
from yc_auth.exceptions import AuthFailureError, ErrorMessages, TooManyRequestsError

log = logging.getLogger(__name__)


class TokenAuth(_TokenAuth):
    @gen.coroutine
    def authenticate_tornado_request(self, request, request_id=None):
        if "X-YaCloud-SubjectToken" not in request.headers:
            raise AuthFailureError(ErrorMessages.Unauthenticated)
        token = request.headers["X-YaCloud-SubjectToken"]

        result = yield self.authenticate(token, request_id)
        raise gen.Return(result)

    @gen.coroutine
    def authenticate(self, token, request_id=None):
        if self.access_service_client is None:
            raise ValueError("Gauthling is no longer supported.")

        try:
            as_response = yield self.access_service_client.authenticate(token, request_id=request_id)
        except (
            yc_as_client.exceptions.BadRequestException,
            yc_as_client.exceptions.UnauthenticatedException,
        ):
            # FIXME: Get the reason of auth failure from AS
            # and convert it to appropriate error (eg. SignatureDoesNotMatchError)
            raise AuthFailureError(ErrorMessages.Unauthenticated)
        auth_ctx = AuthContext.from_access_service_response(token, as_response)

        if not self.check_limits(auth_ctx):
            raise TooManyRequestsError()
        raise gen.Return(auth_ctx)


class TokenAuthWithSignVerification(TokenAuth):
    """DEPRECATED. Use yc_auth_tornado.authentication.TokenAuth."""

    def __init__(self, get_gauthling_client, get_access_service_client):
        import warnings
        warnings.warn(
            "yc_auth_tornado.authentication.TokenAuthWithSignVerification is deprecated. "
            "Use yc_auth_tornado.authentication.TokenAuth instead.",
            DeprecationWarning,
        )

        super(TokenAuthWithSignVerification, self).__init__(get_gauthling_client, get_access_service_client)
