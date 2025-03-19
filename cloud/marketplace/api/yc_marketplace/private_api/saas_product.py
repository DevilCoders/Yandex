"""SAAS product"""
from yc_common.misc import timestamp

from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductAddToCategoryRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductList
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductMetadata
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductOperation
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductPublishRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import \
    SaasProductPutOnPlaceInCategoryRequest
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductResponse
from cloud.marketplace.common.yc_marketplace_common.models.saas_product import SaasProductUpdateRequest


@private_api_handler(
    "GET",
    "/saasProducts/<product_id>",
    context_params=["product_id"],
    response_model=SaasProductResponse,
)
@i18n_traverse()
def get_saas_product(product_id, request_context) -> SaasProductResponse:
    """Return saas product by Id"""
    product = lib.SaasProduct.rpc_get_full(product_id)
    return product.to_api(True)


@private_api_handler(
    "GET",
    "/saasProducts",
    params_model=BaseMktManageListingRequest,
    response_model=SaasProductList,
)
@i18n_traverse()
def list_saas_products(request: BaseMktManageListingRequest, request_context) -> SaasProductList:
    """List saas products"""
    return lib.SaasProduct.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        filter_query=request.filter,
        order_by=request.order_by,
    ).to_api(True)


@private_api_handler(
    "POST",
    "/saasProducts",
    model=SaasProductCreateRequest,
    response_model=SaasProductOperation,
)
def create_saas_product(request: SaasProductCreateRequest, request_context) -> SaasProductOperation:
    return lib.SaasProduct.rpc_create(request)


@private_api_handler(
    "PATCH",
    "/saasProducts/<product_id>",
    model=SaasProductUpdateRequest,
    response_model=SaasProductOperation,
    query_variables=True,
)
def update_saas_product(request: SaasProductUpdateRequest, request_context) -> SaasProductOperation:
    return lib.SaasProduct.rpc_update(request)


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/saasProducts/<product_id>:setOrder",
    model=SaasProductAddToCategoryRequest,
    response_model=SaasProductOperation,
    query_variables=True,
)
def set_order_for_saas_product(request: SaasProductAddToCategoryRequest, request_context) -> SaasProductOperation:
    categories = request.order.items()
    product = lib.SaasProduct.rpc_get(request.product_id)
    lib.Ordering.set_order(request.product_id, categories)
    full_product = lib.SaasProduct.rpc_get_full(request.product_id)
    op = SaasProductOperation({
        "id": product.id,
        "description": "add_to_category",
        "created_at": timestamp(),
        "done": True,
        "metadata": SaasProductMetadata({"saas_product_id": product.id}).to_primitive(),
        "response": full_product.to_api(True),
    })
    return lib.TaskUtils.fake(op, "fake_add_to_category")


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/saasProducts/<product_id>:putOnPlace",
    model=SaasProductPutOnPlaceInCategoryRequest,
    response_model=SaasProductOperation,
    query_variables=True,
)
def put_on_place_saas_product(request: SaasProductPutOnPlaceInCategoryRequest,
                                request_context) -> SaasProductOperation:
    product = lib.SaasProduct.rpc_get(request.product_id)
    lib.Ordering.move_to_position(product.id, request.category_id, request.position)
    full_product = lib.SaasProduct.rpc_get_full(product.id)
    op = SaasProductOperation({
        "id": product.id,
        "description": "add_to_category",
        "created_at": timestamp(),
        "done": True,
        "metadata": SaasProductMetadata({"saas_product_id": product.id}).to_primitive(),
        "response": full_product.to_api(True),
    })
    return lib.TaskUtils.fake(op, "fake_add_to_category")


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/saasProducts/<product_id>:publish",
    model=SaasProductPublishRequest,
    response_model=SaasProductOperation,
    query_variables=True,
)
def publish_saas_product(request: SaasProductPublishRequest, request_context) -> SaasProductOperation:
    product = lib.SaasProduct.rpc_get(request.product_id)
    lib.SaasProduct.rpc_publish(product.id)
    full_product = lib.SaasProduct.rpc_get_full(product.id)
    op = SaasProductOperation({
        "id": product.id,
        "description": "publish",
        "created_at": timestamp(),
        "done": True,
        "metadata": SaasProductMetadata({"saas_product_id": product.id}).to_primitive(),
        "response": full_product.to_api(True),
    })
    return lib.TaskUtils.fake(op, "fake_add_to_category")
