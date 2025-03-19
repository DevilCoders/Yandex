"""SIMPLE product"""
from yc_common.misc import timestamp

from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductAddToCategoryRequest
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductList
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductMetadata
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductPublishRequest
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import \
    SimpleProductPutOnPlaceInCategoryRequest
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.simple_product import SimpleProductUpdateRequest


@private_api_handler(
    "GET",
    "/simpleProducts/<product_id>",
    context_params=["product_id"],
    response_model=SimpleProductResponse,
)
@i18n_traverse()
def get_simple_product(product_id, request_context) -> SimpleProductResponse:
    """Return simple product by Id"""
    product = lib.SimpleProduct.rpc_get_full(product_id)
    return product.to_api(True)


@private_api_handler(
    "GET",
    "/simpleProducts",
    params_model=BaseMktManageListingRequest,
    response_model=SimpleProductList,
)
@i18n_traverse()
def list_simple_products(request: BaseMktManageListingRequest, request_context) -> SimpleProductList:
    """List simple products"""
    return lib.SimpleProduct.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)


@private_api_handler(
    "POST",
    "/simpleProducts",
    model=SimpleProductCreateRequest,
    response_model=SimpleProductOperation,
)
def create_simple_product(request: SimpleProductCreateRequest, request_context) -> SimpleProductOperation:
    return lib.SimpleProduct.rpc_create(request)


@private_api_handler(
    "PATCH",
    "/simpleProducts/<product_id>",
    model=SimpleProductUpdateRequest,
    response_model=SimpleProductOperation,
    query_variables=True,
)
def update_simple_product(request: SimpleProductUpdateRequest, request_context) -> SimpleProductOperation:
    return lib.SimpleProduct.rpc_update(request)


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/simpleProducts/<product_id>:setOrder",
    model=SimpleProductAddToCategoryRequest,
    response_model=SimpleProductOperation,
    query_variables=True,
)
def set_order_for_simple_product(request: SimpleProductAddToCategoryRequest, request_context) -> SimpleProductOperation:
    categories = request.order.items()
    product = lib.SimpleProduct.rpc_get(request.product_id)
    lib.Ordering.set_order(request.product_id, categories)
    full_product = lib.SimpleProduct.rpc_get_full(request.product_id)
    op = SimpleProductOperation({
        "id": product.id,
        "description": "add_to_category",
        "created_at": timestamp(),
        "done": True,
        "metadata": SimpleProductMetadata({"simple_product_id": product.id}).to_primitive(),
        "response": full_product.to_api(True),
    })
    return lib.TaskUtils.fake(op, "fake_add_to_category")


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/simpleProducts/<product_id>:putOnPlace",
    model=SimpleProductPutOnPlaceInCategoryRequest,
    response_model=SimpleProductOperation,
    query_variables=True,
)
def put_on_place_simple_product(request: SimpleProductPutOnPlaceInCategoryRequest,
                                request_context) -> SimpleProductOperation:
    product = lib.SimpleProduct.rpc_get(request.product_id)
    lib.Ordering.move_to_position(product.id, request.category_id, request.position)
    full_product = lib.SimpleProduct.rpc_get_full(product.id)
    op = SimpleProductOperation({
        "id": product.id,
        "description": "add_to_category",
        "created_at": timestamp(),
        "done": True,
        "metadata": SimpleProductMetadata({"simple_product_id": product.id}).to_primitive(),
        "response": full_product.to_api(True),
    })
    return lib.TaskUtils.fake(op, "fake_add_to_category")


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/simpleProducts/<product_id>:publish",
    model=SimpleProductPublishRequest,
    response_model=SimpleProductOperation,
    query_variables=True,
)
def publish_simple_product(request: SimpleProductPublishRequest, request_context) -> SimpleProductOperation:
    product = lib.SimpleProduct.rpc_get(request.product_id)
    lib.SimpleProduct.rpc_publish(product.id)
    full_product = lib.SimpleProduct.rpc_get_full(product.id)
    op = SimpleProductOperation({
        "id": product.id,
        "description": "publish",
        "created_at": timestamp(),
        "done": True,
        "metadata": SimpleProductMetadata({"simple_product_id": product.id}).to_primitive(),
        "response": full_product.to_api(True),
    })
    return lib.TaskUtils.fake(op, "fake_add_to_category")
