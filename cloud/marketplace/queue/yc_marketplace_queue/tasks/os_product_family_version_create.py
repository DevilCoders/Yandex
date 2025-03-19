from typing import Iterable
from typing import Union

from schematics.exceptions import BaseError

from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.validation import ResourceIdType
from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersionResponse
from cloud.marketplace.common.yc_marketplace_common.models.task import OsProductFamilyVersionCreateParams
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.models.results import PublishLogoResult

log = logging.get_logger("yc_marketplace_queue")


def get_version(id: ResourceIdType, url: str) -> OsProductFamilyVersionResponse:
    OsProductFamilyVersion.rpc_set_logo_uri(id, url)
    return OsProductFamilyVersion.rpc_get(id)


def task(depends: Iterable[Task], params, *args, **kwargs) -> Union[TaskResolution, None]:
    log.debug("deps %s", depends)
    try:
        pt = list(filter(lambda t: t.operation_type == "publish_logo", depends))[0]
        d = PublishLogoResult(pt.response)
        d.validate()
        url = d.url
    except (BaseError, IndexError) as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskDepsValidationError",
                "message": "Deps validation failed with error: {}.".format(e),
            })
    try:
        p = OsProductFamilyVersionCreateParams(params["params"], validate=True)
    except BaseException as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })

    try:
        product = get_version(
            p.id,
            url,
        )
    except YcClientError as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": str(e),
            })

    return TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data=product.to_api(True))
