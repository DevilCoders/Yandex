import os
from typing import Iterable
from typing import Union

from schematics.exceptions import BaseError

from yc_common import config
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.clients.compute import ComputeClient
from yc_common.clients.models.operations import OperationV1Beta1
from yc_common.validation import ResourceIdType
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token
from cloud.marketplace.queue.yc_marketplace_queue.models.results import UpdateImagePoolSizeResult

log = logging.get_logger("yc_marketplace_queue")


def check_image_pool_size(operation_id: ResourceIdType) -> OperationV1Beta1:
    organization_id = os.getenv("MARKETPLACE_CLOUD_ID")
    if organization_id is None:
        raise EnvironmentError("Environment variable MARKETPLACE_CLOUD_ID is not set")

    compute_public_client = ComputeClient(
        url=config.get_value("endpoints.compute.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    )

    return compute_public_client.get_operation(operation_id)


def task(depends: Iterable[Task], *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        t = list(filter(lambda x: x.operation_type in ["update_image_pool_size"], depends))[0]
        d = UpdateImagePoolSizeResult(t.response)
        d.validate()
    except (BaseError, IndexError) as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskDepsValidationError",
                "message": "Deps validation failed with error: {}.".format(e),
            })

    try:
        op = check_image_pool_size(
            d.operation_id,
        )
    except YcClientError as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": str(e),
            })

    if op.done:
        if not op.error:
            return TaskResolution.resolve(
                status=TaskResolution.Status.RESOLVED,
                data={},
            )
        else:
            return TaskResolution.resolve(
                status=TaskResolution.Status.FAILED,
                data={
                    "message": op.error.message,
                    "code": op.error.code,
                    "details": op.error.details,
                })
    else:
        log.info("Image pool size is still updating.")
        return None
