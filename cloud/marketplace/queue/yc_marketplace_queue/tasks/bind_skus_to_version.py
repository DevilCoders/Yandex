from typing import Union

from yc_common import config
from yc_common import logging
from yc_common.clients.api import YcClientError
from yc_common.clients.billing import SkuProductLinkRequest
from yc_common.validation import ResourceIdType
from cloud.marketplace.common.yc_marketplace_common.client.billing import BillingPrivateClient
from cloud.marketplace.common.yc_marketplace_common.lib import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.task import BindSkusToVersionParams
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils import metadata_token

log = logging.get_logger("yc_marketplace_queue")


def bind_skus(version_id: ResourceIdType):
    billing_private_client = BillingPrivateClient(
        billing_url=config.get_value("endpoints.billing.url"),
        iam_token=metadata_token.get_instance_metadata_token(),
    )
    version = OsProductFamilyVersion.get(version_id)

    for sku in version.skus:
        billing_private_client.link_sku(sku.id, SkuProductLinkRequest({
            "product_id": version_id,
            "check_formula": sku.check_formula,
        }))


def task(params, *args, **kwargs) -> Union[TaskResolution, None]:
    try:
        p = BindSkusToVersionParams(params["params"])
        p.validate()
    except BaseException as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamsValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })

    try:
        bind_skus(
            p.version_id,
        )
    except YcClientError as e:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "message": str(e),
            })
    return TaskResolution.resolve(
        status=TaskResolution.Status.RESOLVED,
        data={
            "version_id": p.version_id,
        })
