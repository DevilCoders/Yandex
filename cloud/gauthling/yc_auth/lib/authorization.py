import functools

import yc_as_client

from .exceptions import UnauthorizedOperationError


# Attention: `folder_extractor` must be specified for all actions on folder-level resources. In case of
# cloud-level resources `cloud_extractor` must be specified instead.
def authz(
    get_gauthling_client,
    get_access_service_client,
    action,
    token_extractor,
    folder_id_extractor=None,
    cloud_id_extractor=None,
    sa_id_extractor=None,
    request_id_extractor=None,
    ba_id_extractor=None,
):
    """
    Basic authorization decorator.

    It requires client to define extractor functions in order to get required data from request or elsewhere.
    """
    def decorator(func):
        if get_gauthling_client is not None:
            raise ValueError("Gauthling is no longer supported.")

        if get_access_service_client is not None and not callable(get_access_service_client):
            raise TypeError("get_access_service_client must be callable")

        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            token = token_extractor(*args, **kwargs)
            folder_id = folder_id_extractor(*args, **kwargs) if callable(folder_id_extractor) else None
            cloud_id = cloud_id_extractor(*args, **kwargs) if callable(cloud_id_extractor) else None
            sa_id = sa_id_extractor(*args, **kwargs) if callable(sa_id_extractor) else None
            request_id = request_id_extractor(*args, **kwargs) if callable(request_id_extractor) else None
            ba_id = ba_id_extractor(*args, **kwargs) if callable(ba_id_extractor) else None

            access_service_client = get_access_service_client() if callable(get_access_service_client) else None

            authorize(
                None, access_service_client,
                action, token,
                folder_id=folder_id, cloud_id=cloud_id, sa_id=sa_id,
                request_id=request_id, ba_id=ba_id,
            )

            return func(*args, **kwargs)

        return wrapper
    return decorator


# Attention: `folder_id` must be specified for all actions on folder-level resources. In case of
# cloud-level resources `cloud_id` must be specified instead.
def authorize(
    gauthling_client,
    access_service_client,
    action,
    token=None,
    folder_id=None,
    cloud_id=None,
    sa_id=None,
    request_id=None,
    ba_id=None,
    subject=None,
    signature=None,
):
    if gauthling_client is not None:
        raise ValueError("Gauthling is no longer supported.")

    if cloud_id is not None:
        resource = yc_as_client.entities.Resource(
            id=cloud_id,
            type="resource-manager.cloud",
        )
    elif folder_id is not None:
        resource = yc_as_client.entities.Resource(
            id=folder_id,
            type="resource-manager.folder",
        )
    elif sa_id is not None:
        resource = yc_as_client.entities.Resource(
            id=sa_id,
            type="iam.serviceAccount",
        )
    elif ba_id is not None:
        resource = yc_as_client.entities.Resource(
            id=ba_id,
            type="billing.account",
        )
    else:
        raise ValueError("No resource supplied.")

    try:
        access_service_client.authorize(
            iam_token=token,
            subject=subject,
            signature=signature,
            permission=action,
            resource_path=[resource],
            request_id=request_id,
        )
        return
    except (
        yc_as_client.exceptions.BadRequestException,
        yc_as_client.exceptions.PermissionDeniedException,
    ):
        raise UnauthorizedOperationError()
