from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugAddRequest
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugRemoveRequest
from cloud.marketplace.common.yc_marketplace_common.models.product_slug import ProductSlugResponse


@private_api_handler(
    "POST",
    "/productSlugs",
    model=ProductSlugAddRequest,
    response_model=ProductSlugResponse,
)
def add_os_product_slug(request: ProductSlugAddRequest, request_context) -> ProductSlugResponse:
    return lib.ProductSlug.rpc_add_slug(request)


@private_api_handler(
    "DELETE",
    "/productSlugs",
    model=ProductSlugRemoveRequest,
    response_model=ProductSlugResponse,
)
def del_os_product_slug(request: ProductSlugRemoveRequest, request_context) -> ProductSlugResponse:
    return lib.ProductSlug.rpc_del_slug(request)
