import functools
import time

from yc_common import logging
from yc_common.api.request_context import RequestContext
from yc_common.clients import access_service

from yc_auth import authorization

log = logging.get_logger(__name__)


def _log_auth_time(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.monotonic()

        try:
            return func(*args, **kwargs)
        finally:
            auth_time = time.monotonic() - start_time
            if auth_time > 1:
                log.warning("Authorization process took %.1f seconds.", auth_time)

    return wrapper


# Attention:
#
# We can't log authorization time without patching yc_auth.authorization.authorize() because it's used inside of
# yc_auth.authorization.authz decorator.
authorization.authorize = _log_auth_time(authorization.authorize)


def _token_extractor(*args, **kwargs):
    return kwargs["request_context"].auth.token_bytes


def _request_id_extractor(*args, **kwargs):
    return kwargs["request_context"].request_id


def authz(action, folder_extractor=None, cloud_extractor=None, **kwargs):
    return authorization.authz(
        get_gauthling_client=None,
        get_access_service_client=access_service.AccessService.instance,
        action=action,
        token_extractor=_token_extractor,
        folder_id_extractor=folder_extractor,
        cloud_id_extractor=cloud_extractor,
        request_id_extractor=_request_id_extractor,
        **kwargs
    )


# Attention: `folder_id` must be specified for all actions on folder-level resources. In case of
# cloud-level resources `cloud_id` must be specified instead.
def authorize(action, request_context: RequestContext, folder_id=None, cloud_id=None, sa_id=None, **kwargs):
    token = request_context.auth.token_bytes

    return authorization.authorize(
        gauthling_client=None,
        access_service_client=access_service.AccessService.instance(),
        action=action,
        token=token,
        folder_id=folder_id,
        cloud_id=cloud_id,
        request_id=request_context.request_id,
        sa_id=sa_id,
        **kwargs,
    )
