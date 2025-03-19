"""OS product"""
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProduct
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionBatchIdsRequest
from cloud.marketplace.common.yc_marketplace_common.utils import errors


@api_handler_without_auth(
    "GET",
    "/osProducts/<product_id>",
    context_params=["product_id"],
)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def get_public_product(product_id, request_context):
    """Return os product by Id"""

    product = lib.OsProduct.rpc_get_full(product_id)

    if product.status not in OsProduct.Status.PUBLIC:
        raise errors.OsProductIdError()

    return product.to_api(True)


@api_handler_without_auth(
    "GET",
    "/osProducts",
    params_model=BaseMktListingRequest,
    response_model=OsProductList,
)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def list_public_products(request: BaseMktListingRequest, request_context, **kwargs) -> OsProductList:
    """List os products"""
    return lib.OsProduct.rpc_public_list(
        request,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)


@api_handler_without_auth(
    "GET",
    "/osProducts/batch",
    params_model=OsProductFamilyVersionBatchIdsRequest,
)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def get_batch_public_products(request: OsProductFamilyVersionBatchIdsRequest, request_context, **kwargs):
    """Batch resolve os products"""
    ids = [i.strip() for i in request.ids.split(",")]
    return lib.OsProduct.rpc_get_batch(
        version_ids=ids,
    )
