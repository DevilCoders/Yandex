"""OS product"""
from yc_common import config
from yc_common.clients.marketplace_private import CheckUsagePermissionsResponse
from yc_common.misc import timestamp
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product import CheckUsagePermissionsRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductAddToCategoryRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductList
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductMetadata
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductPutOnPlaceInCategoryRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product import OsProductUpdateRequest


@private_api_handler(
    "GET",
    "/osProducts/<product_id>",
    context_params=["product_id"],
    response_model=OsProductResponse,
)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def get_product(product_id, request_context) -> OsProductResponse:
    """Return os product by Id"""
    product = lib.OsProduct.rpc_get_full(product_id)
    return product.to_api(True)


@private_api_handler(
    "GET",
    "/osProducts",
    params_model=BaseMktManageListingRequest,
    response_model=OsProductList,
)
@i18n_traverse(to_sort={"osProductFamilies": lambda x: x["name"]})
def list_products(request: BaseMktManageListingRequest, request_context) -> OsProductList:
    """List os products"""
    return lib.OsProduct.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)


@private_api_handler(
    "POST",
    "/osProducts",
    model=OsProductCreateRequest,
    response_model=OsProductOperation,
)
def create_product(request: OsProductCreateRequest, request_context) -> OsProductOperation:
    return lib.OsProduct.rpc_create(request)


@private_api_handler(
    "PATCH",
    "/osProducts/<os_product_id>",
    model=OsProductUpdateRequest,
    response_model=OsProductOperation,
    query_variables=True,
)
def update_product(request: OsProductUpdateRequest, request_context) -> OsProductOperation:
    return lib.OsProduct.rpc_update(request)


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/osProducts/<os_product_id>:setOrder",
    model=OsProductAddToCategoryRequest,
    response_model=OsProductOperation,
    query_variables=True,
)
def set_order_for_product(request: OsProductAddToCategoryRequest, request_context) -> OsProductOperation:
    categories = request.order.items()
    product = lib.OsProduct.rpc_get(request.os_product_id)
    lib.Ordering.set_order(request.os_product_id, categories)
    full_product = lib.OsProduct.rpc_get_full(request.os_product_id)
    op = OsProductOperation({
        "id": product.id,
        "description": "add_to_category",
        "created_at": timestamp(),
        "done": True,
        "metadata": OsProductMetadata({"os_product_id": product.id}).to_primitive(),
        "response": full_product.to_api(True),
    })

    return lib.TaskUtils.fake(op, "fake_add_to_category")


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/osProducts/<os_product_id>:putOnPlace",
    model=OsProductPutOnPlaceInCategoryRequest,
    response_model=OsProductOperation,
    query_variables=True,
)
def put_on_place_product(request: OsProductPutOnPlaceInCategoryRequest, request_context) -> OsProductOperation:
    product = lib.OsProduct.rpc_get(request.os_product_id)
    lib.Ordering.move_to_position(product.id, request.category_id, request.position)
    full_product = lib.OsProduct.rpc_get_full(product.id)
    op = OsProductOperation({
        "id": product.id,
        "description": "add_to_category",
        "created_at": timestamp(),
        "done": True,
        "metadata": OsProductMetadata({"os_product_id": product.id}).to_primitive(),
        "response": full_product.to_api(True),
    })

    return lib.TaskUtils.fake(op, "fake_add_to_category")


@private_api_handler(
    "POST",
    "/osProducts:checkUsagePermissions",
    model=CheckUsagePermissionsRequest,
    response_model=CheckUsagePermissionsResponse,
)
def check_usage_permissions(request: CheckUsagePermissionsRequest, request_context) -> CheckUsagePermissionsResponse:
    billing_url = config.get_value("endpoints.billing.url")
    return lib.OsProduct.rpc_check_usage_permissions(billing_url, request.cloud_id, request.product_ids)
