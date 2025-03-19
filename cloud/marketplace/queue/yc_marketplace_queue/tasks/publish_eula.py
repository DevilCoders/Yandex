from typing import Union

from cloud.marketplace.common.yc_marketplace_common.lib import Eula
from cloud.marketplace.common.yc_marketplace_common.models.task import PublishEulaParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.models.results import PublishLogoResult
from yc_common import logging
from yc_common.clients.api import YcClientError

log = logging.get_logger("yc_marketplace_queue")


def publish_eula(eula_id: str, target_key: str, target_bucket: str) -> str:
    return Eula.rpc_copy(eula_id, target_key, target_bucket)


def task(params, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        p = PublishEulaParams(params["params"], validate=True)
    except BaseException as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })

    try:
        url = publish_eula(
            p.eula_id,
            p.target_key,
            p.target_bucket,
        )
    except YcClientError as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": str(e),
            })

    return TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data=PublishLogoResult({
            "url": url,
        }).to_primitive())
