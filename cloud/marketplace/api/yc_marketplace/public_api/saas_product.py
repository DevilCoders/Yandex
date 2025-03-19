"""SAAS product"""
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProduct
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductList
from cloud.marketplace.common.yc_marketplace_common.utils import errors


@api_handler_without_auth(
    "GET",
    "/saasProducts/<product_id>",
    context_params=["product_id"],
)
@i18n_traverse()
def get_public_saas_product(product_id, request_context):
    """Return saas product by Id"""
    product = lib.SaasProduct.rpc_get_full(product_id)
    if product.status not in SaasProduct.Status.PUBLIC:
        raise errors.SaasProductIdError()
    return product.to_api(True)


@api_handler_without_auth(
    "GET",
    "/saasProducts",
    params_model=BaseMktListingRequest,
    response_model=SaasProductList,
)
@i18n_traverse()
def list_public_saas_products(request: BaseMktListingRequest, request_context, **kwargs) -> SaasProductList:
    """List saas products"""
    return lib.SaasProduct.rpc_public_list(
        request,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)
