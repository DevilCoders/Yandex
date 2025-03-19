"""Saas product"""
from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authorize_action
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductList
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductUpdateRequest


@api_handler(
    "GET",
    "/manage/saasProducts/<product_id>",
    context_params=["product_id"],
    response_model=SaasProductResponse,
)
@i18n_traverse()
def get_saas_product(product_id, request_context) -> SaasProductResponse:
    """Return saas product by Id"""
    product = lib.SaasProduct.rpc_get_full(product_id)
    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_OS_PRODUCT,
        request_context=request_context,
        ba_id=product.billing_account_id)
    return product.to_api(True)


@api_handler(
    "GET",
    "/manage/saasProducts",
    params_model=BaseMktManageListingRequest,
    response_model=SaasProductList,
)
@authz(action_name=ActionNames.LIST_OS_PRODUCTS)
@i18n_traverse()
def list_saas_products(request: BaseMktManageListingRequest, request_context) -> SaasProductList:
    """List saas products"""
    return lib.SaasProduct.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)


@api_handler(
    "POST",
    "/manage/saasProducts",
    model=SaasProductCreateRequest,
    response_model=SaasProductOperation,
)
@authz(action_name=ActionNames.CREATE_OS_PRODUCT)
@i18n_traverse()
def create_saas_product(request: SaasProductCreateRequest, request_context) -> SaasProductOperation:
    return lib.SaasProduct.rpc_create(request)


@api_handler(
    "PATCH",
    "/manage/saasProducts/<product_id>",
    model=SaasProductUpdateRequest,
    response_model=SaasProductOperation,
    query_variables=True,
)
@i18n_traverse()
def update_saas_product(request: SaasProductUpdateRequest, request_context) -> SaasProductOperation:
    product = lib.SaasProduct.rpc_get_full(request.product_id)
    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.UPDATE_OS_PRODUCT,
        request_context=request_context,
        ba_id=product.billing_account_id)
    return lib.SaasProduct.rpc_update(request)
