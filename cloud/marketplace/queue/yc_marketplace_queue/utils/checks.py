from typing import Type

from schematics.exceptions import BaseError

from yc_common import logging
from yc_common.models import Model
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.exceptions import TaskParamValidationError

log = logging.get_logger("yc_marketplace_queue")


def get_params(params: dict, params_class: Type[Model]) -> Model:
    try:
        params_obj = params_class(params)
        params_obj.validate()
    except BaseError as e:
        log.exception("%s validation failed.", params_class.__name__)

        raise TaskParamValidationError(
            resolution_resolved=TaskResolution.resolve(
                status=TaskResolution.Status.FAILED,
                data={
                    "code": "TaskParamValidationError",
                    "message": "Params validation failed with error: {}.".format(e),
                }),
        )

    return params_obj
