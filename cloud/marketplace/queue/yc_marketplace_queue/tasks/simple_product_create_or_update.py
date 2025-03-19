from typing import Iterable
from typing import Optional
from typing import Union

from schematics.exceptions import BaseError
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.validation import ResourceIdType

from cloud.marketplace.common.yc_marketplace_common.lib import SimpleProduct
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.task import OsProductCreateOrUpdateParams
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.models.results import PublishLogoResult

log = logging.get_logger("yc_marketplace_queue")


def get_product(id: ResourceIdType, url: Optional[str]) -> SimpleProductResponse:
    if url is not None:
        SimpleProduct.rpc_set_logo_uri(id, url)
    return SimpleProduct.rpc_get_full(id)


def task(depends: Iterable[Task], params, *args, **kwargs) -> Union[TaskResolution, None]:
    deps_list = list(depends)
    log.debug("deps %s", deps_list)
    url = None
    if deps_list:
        try:
            pt = list(filter(lambda t: t.operation_type == "publish_logo", deps_list))[0]
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
        p = OsProductCreateOrUpdateParams(params["params"], validate=True)
    except BaseException as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })
    try:
        product = get_product(
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
