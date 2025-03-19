from typing import Union

from schematics.exceptions import BaseError

from yc_common import config
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.clients.compute import ComputeClient
from yc_common.exceptions import ApiError
from yc_common.exceptions import LogicalError
from cloud.marketplace.common.yc_marketplace_common.models.task import DeleteImagePoolParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token

log = logging.get_logger("yc_marketplace_queue")


def delete_image_pool(image_id):
    compute_public_client = ComputeClient(
        url=config.get_value("endpoints.compute.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    )

    create_op = compute_public_client.delete_disk_pooling(image_id=image_id)

    log.info("Delete image pool for image '{image_id}".format(
        image_id=image_id,
    ))
    return create_op


def task(params, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        p = DeleteImagePoolParams(params["params"])
        p.validate()
    except BaseError as e:
        log.exception("Delete image pool params validation failed")
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })
    try:
        create_op = delete_image_pool(
            p.image_id,
        )
    except (ApiError, LogicalError, YcClientError) as e:
        log.exception("Can not copy update image pool size")
        try:
            message = e.message
        except AttributeError:
            message = str(e)

        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": message,
            })

    log.debug("Delete image pool operation %s", create_op)

    return TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={},
    )
