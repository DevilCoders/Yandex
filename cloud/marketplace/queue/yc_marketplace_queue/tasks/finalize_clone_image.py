import os
from typing import Iterable
from typing import Union

from schematics.exceptions import BaseError

from yc_common import config
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.clients.compute import ComputeClient
from yc_common.clients.models import images
from yc_common.exceptions import ApiError
from yc_common.validation import ResourceIdType
from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersion as OsProductFamilyVersionScheme
from cloud.marketplace.common.yc_marketplace_common.models.task import FinalizeImageParams
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token
from cloud.marketplace.queue.yc_marketplace_queue.models.results import FinalizeImageResult
from cloud.marketplace.queue.yc_marketplace_queue.models.results import MoveImageResult

log = logging.get_logger("yc_marketplace_queue")


def check_image(image_id: ResourceIdType) -> images.Image:
    organization_id = os.getenv("MARKETPLACE_CLOUD_ID")
    if organization_id is None:
        raise EnvironmentError("Environment variable MARKETPLACE_CLOUD_ID is not set")

    compute_public_client = ComputeClient(
        url=config.get_value("endpoints.compute.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    )
    image = compute_public_client.get_image(
        image_id=image_id,
    )

    log.info("Image {new} has status {status}".format(new=image_id, status=image.status))

    return image


def task(depends: Iterable[Task], params, *args, **kwargs) -> Union[TaskResolution, None]:
    log.debug("deps %s", depends)
    try:
        p = FinalizeImageParams(params["params"], validate=True)
    except BaseException as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskDepsValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })
    try:
        t = list(filter(lambda x: x.operation_type in ("start_clone_image", "start_publish_image", "start_create_task"),
                        depends))[0]
        d = MoveImageResult(t.response)
        d.validate()
    except (BaseError, IndexError) as e:
        log.debug("version -> error")
        OsProductFamilyVersion.update(p.version_id, {"status": OsProductFamilyVersionScheme.Status.ERROR})
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskDepsValidationError",
                "message": "Deps validation failed with error: {}.".format(e),
            })

    try:
        image = check_image(
            d.new_image_id,
        )
    except YcClientError as e:
        OsProductFamilyVersion.update(p.version_id, {"status": OsProductFamilyVersionScheme.Status.ERROR})
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": str(e),
            })
    except ApiError:
        return None

    if image.status in (images.Image.Status.ERROR, images.Image.Status.DELETING):
        OsProductFamilyVersion.update(p.version_id, {"status": OsProductFamilyVersionScheme.Status.ERROR})
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": "Creating image ended with status code {}".format(image.status),
            })

    elif image.status == images.Image.Status.READY:
        OsProductFamilyVersion.update(p.version_id, {"status": p.target_status}, propagate=True)
        return TaskResolution.resolve(
            status=TaskResolution.Status.RESOLVED,
            data=FinalizeImageResult({
                "image_id": image.id,
                "status": image.status,
            }).to_primitive(),
        )
    else:
        log.info("Image state {}. Still waiting".format(image.status))

        return None
