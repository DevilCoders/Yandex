from typing import Union

from yc_common import logging
from cloud.marketplace.common.yc_marketplace_common.lib import PartnerRequest
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequest as AccountRequestsScheme
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution

log = logging.get_logger("yc_marketplace_queue")


def task(params, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        request = PartnerRequest.get(params["params"]["id"])
    except BaseException as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskValidationError",
                "message": "Can't get request with error: {}.".format(e),
            })

    if request.status == AccountRequestsScheme.Status.CLOSE:
        return TaskResolution.resolve(
            status=TaskResolution.Status.CANCELED,
            data={
                "code": "CanceledError",
                "message": "Canceled by user",
            })

    if request.status == AccountRequestsScheme.Status.APPROVE:
        return TaskResolution.resolve(status=TaskResolution.Status.RESOLVED, data={})
    if request.status == AccountRequestsScheme.Status.DECLINE:
        return TaskResolution.resolve(status=TaskResolution.Status.FAILED, data={
            "code": "RejectedError",
            "message": "Rejected by admin",
        })
