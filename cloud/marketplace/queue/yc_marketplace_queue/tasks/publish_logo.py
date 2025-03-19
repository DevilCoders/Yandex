from typing import Union

from yc_common import logging
from yc_common.clients.api import YcClientError
from cloud.marketplace.common.yc_marketplace_common.lib import Avatar
from cloud.marketplace.common.yc_marketplace_common.models.task import PublishLogoParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.models.results import PublishLogoResult

log = logging.get_logger("yc_marketplace_queue")


def publish_logo(avatar_id: str, target_key: str, target_bucket: str, rewrite: bool = False) -> str:
    return Avatar.rpc_copy(avatar_id, target_key, target_bucket, rewrite=rewrite)


def task(params, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        p = PublishLogoParams(params["params"], validate=True)
    except BaseException as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })

    try:
        url = publish_logo(
            p.avatar_id,
            p.target_key,
            p.target_bucket,
            p.rewrite,
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
