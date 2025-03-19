import yc_as_client
from tornado import gen
from yc_auth.exceptions import UnauthorizedOperationError


# Attention: `folder_id` must be specified for all actions on folder-level resources. In case of
# cloud-level resources `cloud_id` must be specified instead.
@gen.coroutine
def authorize(
    gauthling_client,
    access_service_client,
    action,
    token=None,
    folder_id=None,
    cloud_id=None,
    request_id=None,
    subject=None,
    signature=None,
):
    if access_service_client is None:
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
    else:
        raise ValueError("No resource supplied.")

    try:
        yield access_service_client.authorize(
            iam_token=token,
            subject=subject,
            signature=signature,
            permission=action,
            resource_path=[resource],
            request_id=request_id,
        )
        raise gen.Return()
    except (
        yc_as_client.exceptions.BadRequestException,
        yc_as_client.exceptions.PermissionDeniedException,
    ):
        raise UnauthorizedOperationError()

    raise gen.Return()
