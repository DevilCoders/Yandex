"""Simple product"""
from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authorize_action
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductList
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductUpdateRequest


@api_handler(
    "GET",
    "/manage/simpleProducts/<product_id>",
    context_params=["product_id"],
    response_model=SimpleProductResponse,
)
@i18n_traverse()
def get_simple_product(product_id, request_context) -> SimpleProductResponse:
    """Return simple product by Id"""
    product = lib.SimpleProduct.rpc_get_full(product_id)
    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_OS_PRODUCT,
        request_context=request_context,
        ba_id=product.billing_account_id)
    return product.to_api(True)


@api_handler(
    "GET",
    "/manage/simpleProducts",
    params_model=BaseMktManageListingRequest,
    response_model=SimpleProductList,
)
@authz(action_name=ActionNames.LIST_OS_PRODUCTS)
@i18n_traverse()
def list_simple_products(request: BaseMktManageListingRequest, request_context) -> SimpleProductList:
    """List simple products"""
    return lib.SimpleProduct.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)


@api_handler(
    "POST",
    "/manage/simpleProducts",
    model=SimpleProductCreateRequest,
    response_model=SimpleProductOperation,
)
@authz(action_name=ActionNames.CREATE_OS_PRODUCT)
@i18n_traverse()
def create_simple_product(request: SimpleProductCreateRequest, request_context) -> SimpleProductOperation:
    return lib.SimpleProduct.rpc_create(request)


@api_handler(
    "PATCH",
    "/manage/simpleProducts/<product_id>",
    model=SimpleProductUpdateRequest,
    response_model=SimpleProductOperation,
    query_variables=True,
)
@i18n_traverse()
def update_simple_product(request: SimpleProductUpdateRequest, request_context) -> SimpleProductOperation:
    product = lib.SimpleProduct.rpc_get_full(request.product_id)
    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.UPDATE_OS_PRODUCT,
        request_context=request_context,
        ba_id=product.billing_account_id)
    return lib.SimpleProduct.rpc_update(request)
