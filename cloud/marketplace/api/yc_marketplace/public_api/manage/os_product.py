"""OS product"""

from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authorize_action
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductList
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductUpdateRequest


@api_handler(
    "GET",
    "/manage/osProducts/<product_id>",
    context_params=["product_id"],
    response_model=OsProductResponse,
)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def get_product(product_id, request_context) -> OsProductResponse:
    """Return os product by Id"""

    product = lib.OsProduct.rpc_get_full(product_id)

    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_OS_PRODUCT,
        request_context=request_context,
        ba_id=product.billing_account_id)

    return product.to_api(True)


@api_handler(
    "GET",
    "/manage/osProducts",
    params_model=BaseMktManageListingRequest,
    response_model=OsProductList,
)
@authz(action_name=ActionNames.LIST_OS_PRODUCTS)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def list_products(request: BaseMktManageListingRequest, request_context) -> OsProductList:
    """List os products"""
    return lib.OsProduct.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)


@api_handler(
    "POST",
    "/manage/osProducts",
    model=OsProductCreateRequest,
    response_model=OsProductOperation,
)
@authz(action_name=ActionNames.CREATE_OS_PRODUCT)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def create_product(request: OsProductCreateRequest, request_context) -> OsProductOperation:
    return lib.OsProduct.rpc_create(request)


@api_handler(
    "PATCH",
    "/manage/osProducts/<os_product_id>",
    model=OsProductUpdateRequest,
    response_model=OsProductOperation,
    query_variables=True,
)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def update_product(request: OsProductUpdateRequest, request_context) -> OsProductOperation:
    product = lib.OsProduct.rpc_get_full(request.os_product_id)
    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.UPDATE_OS_PRODUCT,
        request_context=request_context,
        ba_id=product.billing_account_id)
    return lib.OsProduct.rpc_update(request)
