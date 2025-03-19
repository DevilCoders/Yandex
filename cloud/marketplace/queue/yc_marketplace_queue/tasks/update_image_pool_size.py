from typing import Union

from schematics.exceptions import BaseError

from yc_common import config
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.clients.compute import ComputeClient
from yc_common.exceptions import ApiError
from yc_common.exceptions import LogicalError
from yc_common.misc import drop_none
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.models.task import UpdateImagePoolSizeParams
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token
from cloud.marketplace.queue.yc_marketplace_queue.models.results import FinalizeImageResult
from cloud.marketplace.queue.yc_marketplace_queue.models.results import UpdateImagePoolSizeResult

log = logging.get_logger("yc_marketplace_queue")


def update_image_pool_size(image_id, pool_size: int):
    compute_public_client = ComputeClient(
        url=config.get_value("endpoints.compute.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    )

    img_kwargs = drop_none({
        "image_id": image_id,
        "disk_count": pool_size,
        "type_id": "network-ssd",
    })

    create_op = compute_public_client.update_disk_pooling(**img_kwargs)

    log.info("Update image pool size for image '{image_id}' set {pool_size} netword-ssd type, op id {op_id}".format(
        image_id=image_id,
        pool_size=pool_size,
        op_id=create_op,
    ))

    # Compute cannot deprecate network-nvme on their side
    # So we explicitly tell pooling to create network-hdd and network-ssd disks
    # Calling handler twice
    second_img_kwargs = drop_none({
        "image_id": image_id,
        "disk_count": pool_size,
        "type_id": "network-hdd",
    })

    create_op_2 = compute_public_client.update_disk_pooling(**second_img_kwargs)

    log.info("Update image pool size for image '{image_id}' set {pool_size} network-hdd type, op id {op_id}".format(
        image_id=image_id,
        pool_size=pool_size,
        op_id=create_op_2,
    ))

    return create_op_2


def task(depends, params, *args, **kwargs) -> Union[TaskResolution, None]:
    deps_list = list(depends)
    log.debug("deps %s", deps_list)
    try:
        pt = list(filter(lambda t: t.operation_type == "finalize_clone_image", deps_list))[0]
        d = FinalizeImageResult(pt.response)
        d.validate()
    except (BaseError, IndexError) as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskDepsValidationError",
                "message": "Deps validation failed with error: {}.".format(e),
            })
    try:
        p = UpdateImagePoolSizeParams(params["params"])
        p.validate()
    except BaseError as e:
        log.exception("Update image pool size params validation failed")
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })
    # Создаем новый image
    try:
        create_op = update_image_pool_size(
            d.image_id,
            p.pool_size,
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

    log.debug("Update image pool size operation %s", create_op)

    return TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data=UpdateImagePoolSizeResult({
            "operation_id": create_op.id,
        }).to_primitive())
