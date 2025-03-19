"""SIMPLE product"""
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProduct
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductList
from cloud.marketplace.common.yc_marketplace_common.utils import errors


@api_handler_without_auth(
    "GET",
    "/simpleProducts/<product_id>",
    context_params=["product_id"],
)
@i18n_traverse()
def get_public_simple_product(product_id, request_context):
    """Return simple product by Id"""
    product = lib.SimpleProduct.rpc_get_full(product_id)
    if product.status not in SimpleProduct.Status.PUBLIC:
        raise errors.SimpleProductIdError()
    return product.to_api(True)


@api_handler_without_auth(
    "GET",
    "/simpleProducts",
    params_model=BaseMktListingRequest,
    response_model=SimpleProductList,
)
@i18n_traverse()
def list_public_simple_products(request: BaseMktListingRequest, request_context, **kwargs) -> SimpleProductList:
    """List simple products"""
    return lib.SimpleProduct.rpc_public_list(
        request,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)
