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


def authz(service, action, yrn_extractor=None, project_extractor=None, organization_extractor=None):
    _action = "{0}::{1}".format(service, action)
    return authorization.authz(
        get_gauthling_client=None,
        get_access_service_client=access_service.AccessService.instance,
        action=_action,
        token_extractor=_token_extractor,
        folder_id_extractor=project_extractor,
        cloud_id_extractor=organization_extractor,
    )


# Attention: `project_id` must be specified for all actions on project-level resources. In case of
# organization-level resources `organization_id` must be specified instead.
def authorize(service, action, request_context: RequestContext, project_id=None, organization_id=None, yrn=None):
    token = request_context.auth.token_bytes

    _action = "{0}::{1}".format(service, action)
    return authorization.authorize(
        gauthling_client=None,
        access_service_client=access_service.AccessService.instance(),
        action=_action,
        token=token,
        folder_id=project_id, cloud_id=organization_id,
    )
