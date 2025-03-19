import typing

from yc_auth import authentication

from yc_common.clients import access_service
from yc_common.models import Model, ModelType, StringType


class AuthenticationContext(Model):
    class User(Model):
        Type = authentication.AuthContext.User.Type

        id = StringType(required=True)
        type = StringType(required=True)
        name = StringType(required=True)
        passport_uid = StringType()
        folder_id = StringType()

        def is_service_account(self):
            return self.type == self.Type.SERVICE_ACCOUNT

    user = ModelType(User, required=True)
    token_bytes = StringType(required=True)


class TokenAuthWithSignVerification(authentication.TokenAuthWithSignVerification):
    def __init__(self, service_name, **kwargs):
        super().__init__(
            None,
            access_service.AccessService.instance,
            service_name,
            **kwargs,
        )


YcAuthMethodTypes = typing.Union[
    TokenAuthWithSignVerification,
    authentication.TokenAuth,
]
