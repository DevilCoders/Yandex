import logging

import yc_as_client.entities
import yc_as_client.exceptions

from .exceptions import AuthFailureError, ErrorMessages, TooManyRequestsError

log = logging.getLogger(__name__)


REQUIRED_HEADERS = {"x-yacloud-signature", "x-yacloud-requesttime", "x-yacloud-signkeyservice",
                    "x-yacloud-signkeydate", "x-yacloud-signmethod", "x-yacloud-signedheaders",
                    "x-yacloud-subjecttoken"}
EMPTY_SHA256_HASH = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

TIMESTAMP_FMT = "%Y%m%dT%H%M%SZ"


class AuthContext(object):
    """Contains data about user"""

    class User:
        class Type:
            USER_ACCOUNT = "user_account"
            SERVICE_ACCOUNT = "service_account"
            SYSTEM = "system"
            ANONYMOUS_ACCOUNT = "anonymous_account"


        @classmethod
        def from_access_service_response(cls, as_response):
            result = cls()
            if isinstance(as_response, yc_as_client.entities.AnonymousAccountSubject):
                result.type = cls.Type.ANONYMOUS_ACCOUNT
            elif isinstance(as_response, yc_as_client.entities.UserAccountSubject):
                result.type = cls.Type.USER_ACCOUNT
                result.id = as_response.id
            elif isinstance(as_response, yc_as_client.entities.ServiceAccountSubject):
                result.type = cls.Type.SERVICE_ACCOUNT
                result.id = as_response.id
                result.folder_id = as_response.folder_id
            return result

        def __init__(self, id=None, name=None, folder_id=None, type=Type.USER_ACCOUNT):
            self.id = id
            self.type = type
            self.name = name
            # NOTE: This field will be non-null only for service accounts.
            self.folder_id = folder_id if folder_id else None   # convert empty string to None

    @classmethod
    def from_access_service_response(cls, token, as_response):
        user = cls.User.from_access_service_response(as_response)
        return cls(user=user, token=token)

    def __init__(self, user=None, token=None):
        self.user = user
        self.token = token


class BaseAuthMethod(object):
    def authenticate_flask_request(self, request):
        raise NotImplementedError

    def authenticate_aiohttp_request(self, request):
        raise NotImplementedError


class TokenAuth(BaseAuthMethod):
    """YC token-based authentication"""

    def __init__(self, get_gauthling_client, get_access_service_client, check_limits=lambda *args: True):
        if get_gauthling_client is not None:
            raise ValueError("Gauthling is no longer supported.")

        if get_access_service_client is not None and not callable(get_access_service_client):
            raise TypeError("get_access_service_client must be callable")

        self.get_access_service_client = get_access_service_client
        self.check_limits = check_limits

    @property
    def access_service_client(self):
        return self.get_access_service_client() if self.get_access_service_client is not None else None

    def authenticate_flask_request(self, request):
        if "X-YaCloud-SubjectToken" not in request.headers:
            raise AuthFailureError(ErrorMessages.Unauthenticated)
        token = request.headers["X-YaCloud-SubjectToken"]
        request_id = request.headers.get("X-Request-ID")

        return self.authenticate(token, request_id=request_id)

    def authenticate_aiohttp_request(self, request):
        if "X-YaCloud-SubjectToken" not in request.headers:
            raise AuthFailureError(ErrorMessages.Unauthenticated)
        token = request.headers["X-YaCloud-SubjectToken"]
        request_id = request.headers.get("X-Request-ID")

        return self.authenticate(token, request_id=request_id)

    def authenticate(self, token, request_id=None):
        try:
            response = self.access_service_client.authenticate(token, request_id=request_id)
        except (
            yc_as_client.exceptions.BadRequestException,
            yc_as_client.exceptions.UnauthenticatedException,
        ):
            # FIXME: Get the reason of auth failure from AS
            # and convert it to appropriate error (eg. SignatureDoesNotMatchError)
            raise AuthFailureError(ErrorMessages.Unauthenticated)
        auth_ctx = AuthContext.from_access_service_response(token, response)

        if not self.check_limits(auth_ctx):
            raise TooManyRequestsError()
        return auth_ctx


class TokenAuthWithSignVerification(TokenAuth):
    """DEPRECATED. Use yc_auth.authentication.TokenAuth."""

    def __init__(
        self,
        get_gauthling_client, get_access_service_client,
        service_name, check_limits=lambda *args: True,
    ):
        import warnings
        warnings.warn(
            "yc_auth.authentication.TokenAuthWithSignVerification is deprecated. "
            "Use yc_auth.authentication.TokenAuth instead.",
            DeprecationWarning,
        )

        super(TokenAuthWithSignVerification, self).__init__(
            get_gauthling_client, get_access_service_client,
            check_limits,
        )
