import os
from typing import Union

from schematics.exceptions import BaseError

from yc_common import config
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.clients.compute import ComputeClient
from yc_common.misc import drop_none
from yc_common.misc import timestamp
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.task import StartCloneImageParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token
from cloud.marketplace.queue.yc_marketplace_queue.models.results import MoveImageResult

log = logging.get_logger("yc_marketplace_queue")


def create_image(source_image_id, target_folder_id, name, product_ids, resource_spec):
    organization_id = os.getenv("MARKETPLACE_CLOUD_ID")
    if organization_id is None:
        raise EnvironmentError("Environment variable MARKETPLACE_CLOUD_ID is not set")

    compute_public_client = ComputeClient(
        url=config.get_value("endpoints.compute.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    )
    source_image = compute_public_client.get_image(source_image_id)
    product_ids = list(set(product_ids) - set(source_image.product_ids))

    if name is None:
        name = source_image.name

    name = "{}-{}".format(name[:51], timestamp())

    requirements = {}
    if resource_spec and resource_spec.network_interfaces:
        requirements = {
            "min_network_interfaces": str(resource_spec.network_interfaces.min),
            "max_network_interfaces": str(resource_spec.network_interfaces.max),
        }

    img_kwargs = drop_none({
        "name": name,
        "image_id": source_image_id,
        "description": source_image.description,
        "family": source_image.family if source_image.family else None,
        "product_ids": product_ids if product_ids else None,
        "override_product_ids": True,
        "os_type": source_image.os.type,
        "min_disk_size": source_image.min_disk_size,
        "labels": source_image.labels,
        "requirements": requirements,
    })

    create_op = compute_public_client.create_image(target_folder_id, **img_kwargs)

    log.info(
        "Creating image '{name}'. Source image id: {source}. New image {new} in folder: {folder}. OS {os}".format(
            name=name,
            source=source_image_id,
            new=create_op.metadata.image_id,
            folder=target_folder_id,
            os=source_image.os.type,
        ),
    )
    return create_op


def task(params, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        p = StartCloneImageParams(params["params"])
        p.validate()
    except BaseError as e:
        log.exception("Clone Image Params Validation failed")
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })
    # Создаем новый image
    try:
        create_op = create_image(
            p.source_image_id,
            p.target_folder_id,
            p.name,
            p.product_ids,
            p.resource_spec,
        )
    except YcClientError as e:
        log.exception("Can not copy image")
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": str(e),
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
