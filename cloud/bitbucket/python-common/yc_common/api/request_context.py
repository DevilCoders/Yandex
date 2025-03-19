from yc_common.models import Model, ModelType, BooleanType, StringType

from .authentication import AuthenticationContext


class RequestContext(Model):
    request_id = StringType(required=True)
    idempotence_id = StringType()

    auth = ModelType(AuthenticationContext)

    is_internal = BooleanType(default=False)

    peer_address = StringType()
    remote_address = StringType()
    user_agent = StringType()
