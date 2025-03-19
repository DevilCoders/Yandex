import os
from typing import Union

from schematics.exceptions import BaseError

from yc_common import config
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.clients.compute import ComputeClient
from yc_common.misc import drop_none
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.task import StartCreateImageParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token
from cloud.marketplace.queue.yc_marketplace_queue.models.results import MoveImageResult

log = logging.get_logger("yc_marketplace_queue")


CLOUD_ID = os.getenv("MARKETPLACE_CLOUD_ID")


def task(params, *args, **kwargs) -> Union[TaskResolution, None]:
    if CLOUD_ID is None:
        raise EnvironmentError("Environment variable MARKETPLACE_CLOUD_ID is not set")

    try:
        p = StartCreateImageParams(params["params"])
        p.validate()
    except BaseError as e:
        log.exception("Create Image Params Validation failed")
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })
    # Создаем новый image
    try:
        compute_public_client = ComputeClient(
            url=config.get_value("endpoints.compute.url"),
            iam_token=metadata_token.get_instance_metadata_token(),
        )

        img_kwargs = drop_none({
            "name": p.name,
            "description": p.description,
            "family": p.family,
            "product_ids": p.product_ids if len(p.product_ids) else [],
            "override_product_ids": True,
            "os_type": p.os_type,
            "min_disk_size": p.min_disk_size,
            "labels": p.labels,
            "requirements": p.requirements,
            p.field: p.source,
        })
        create_op = compute_public_client.create_image(p.target_folder_id, **img_kwargs)

        log.info("Creating image %s", p.name)
    except YcClientError as e:
        log.exception("Can not copy image")
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": str(e),
            })

    log.debug("Create image operation %s", create_op)
    new_image_id = create_op.metadata.image_id
    if p.version_id is not None:
        lib.OsProductFamilyVersion.update(p.version_id, {
            "image_id": new_image_id,
        })

    return TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data=MoveImageResult({
            "new_image_id": new_image_id,
        }).to_primitive())
