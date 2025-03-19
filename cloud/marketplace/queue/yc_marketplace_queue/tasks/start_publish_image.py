import os
from typing import Union

from schematics.exceptions import BaseError

from yc_common import config
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.clients.compute import ComputeClient
from yc_common.exceptions import ApiError
from yc_common.exceptions import LogicalError
from yc_common.misc import drop_none
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.task import StartPublishVersionParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token
from cloud.marketplace.queue.yc_marketplace_queue.models.results import MoveImageResult

log = logging.get_logger("yc_marketplace_queue")


def move_image(version_id, target_folder_id):
    organization_id = os.getenv("MARKETPLACE_CLOUD_ID")
    if organization_id is None:
        raise EnvironmentError("Environment variable MARKETPLACE_CLOUD_ID is not set")

    compute_public_client = ComputeClient(
        url=config.get_value("endpoints.compute.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    )
    version = lib.OsProductFamilyVersion.get(version_id)
    if version.status != OsProductFamilyVersion.Status.ACTIVATING:
        raise LogicalError("Can not publish version from status '{}'".format(version.status))

    source_image = compute_public_client.get_image(version.image_id)

    img_kwargs = drop_none({
        "name": source_image.name,
        "image_id": version.image_id,
        "description": source_image.description,
        "family": source_image.family if source_image.family else None,
        "os_type": source_image.os.type,
        "min_disk_size": source_image.min_disk_size,
        "labels": source_image.labels,
        "requirements": source_image.requirements,
    })

    create_op = compute_public_client.create_image(target_folder_id, **img_kwargs)

    log.info(
        "Creating image '{name}'. Source image id: {source}. New image {new} in folder: {folder}. OS {os}".format(
            name=source_image.name,
            source=version.image_id,
            new=create_op.metadata.image_id,
            folder=target_folder_id,
            os=source_image.os.type,
        ),
    )
    return create_op


def task(params, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        p = StartPublishVersionParams(params["params"])
        p.validate()
    except BaseError as e:
        log.exception("Start Publish Version Params Validation failed")
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })
    # Создаем новый image
    try:
        create_op = move_image(
            p.version_id,
            p.target_folder_id,
        )
    except (ApiError, LogicalError, YcClientError) as e:
        log.exception("Can not copy image")
        try:
            message = e.message
        except AttributeError:
            message = str(e)

        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": message,
            })

    log.debug("Create image operation %s", create_op)
    new_image_id = create_op.metadata.image_id
    lib.OsProductFamilyVersion.update(p.version_id, {
        "image_id": new_image_id,
    })

    return TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data=MoveImageResult({
            "new_image_id": new_image_id,
        }).to_primitive())
