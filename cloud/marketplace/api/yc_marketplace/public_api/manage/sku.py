from yc_common import config
from yc_common.clients.billing import CreateSkuRequest

from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authorize_action
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.billing.pricing import PricingVersion
from cloud.marketplace.common.yc_marketplace_common.models.billing.sku import SkuList
from cloud.marketplace.common.yc_marketplace_common.models.billing.sku import SkuPublicView


@api_handler(
    "GET",
    "/manage/skus/<sku_id>",
    context_params=["sku_id"],
    response_model=SkuPublicView,
)
@i18n_traverse()
def get_sku(sku_id, request_context) -> SkuPublicView:
    billing_url = config.get_value("endpoints.billing.url")
    sku = lib.Billing.get_sku(billing_url, sku_id)
    publisher = lib.Publisher.rpc_get_by_publisher_account(sku.publisher_account_id)

    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_SKU,
        request_context=request_context,
        ba_id=publisher.billing_account_id)

    return sku.to_api(True)


@api_handler(
    "GET",
    "/manage/skus",
    params_model=BaseMktManageListingRequest,
    response_model=SkuList,
)
@authz(action_name=ActionNames.LIST_SKUS)
@i18n_traverse()
def list_skus(request: BaseMktManageListingRequest, request_context) -> SkuList:
    billing_url = config.get_value("endpoints.billing.url")
    publisher = lib.Publisher.rpc_get_by_ba(request.billing_account_id)

    filter_query = "publisher_account_id='{}'".format(publisher.billing_publisher_account_id)
    return lib.Billing.list_skus(billing_url, filter_query).to_api(True)


@api_handler(
    "POST",
    "/manage/skus",
    model=CreateSkuRequest,
    response_model=SkuPublicView,
)
@authz(action_name=ActionNames.CREATE_SKU)
@i18n_traverse()
def create_sku(request: CreateSkuRequest, request_context) -> SkuPublicView:
    billing_url = config.get_value("endpoints.billing.url")

    return lib.Billing.create_sku(billing_url, request).to_api(True)


@api_handler(
    "PATCH",
    "/manage/skus/<sku_id>",
    model=PricingVersion,
    context_params=["sku_id"],
    response_model=SkuPublicView,
)
@i18n_traverse()
def update_sku(sku_id, request: PricingVersion, request_context) -> SkuPublicView:
    billing_url = config.get_value("endpoints.billing.url")
    sku = lib.Billing.get_sku(billing_url, sku_id)
    publisher = lib.Publisher.rpc_get_by_publisher_account(sku.publisher_account_id)

    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.UPDATE_SKU,
        request_context=request_context,
        ba_id=publisher.billing_account_id)

    return lib.Billing.add_pricing_version(billing_url, sku_id, request).to_api(True)
