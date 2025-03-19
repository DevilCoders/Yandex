"""Category"""
from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from yc_common.clients.models.base import BasePublicListingRequest
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.category import Category
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryList
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryListingRequest


@api_handler_without_auth(
    "GET",
    "/categories",
    params_model=CategoryListingRequest,
    response_model=CategoryList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_categories(request: CategoryListingRequest, request_context) -> CategoryList:
    """List categories"""

    return lib.Category.rpc_list(
        request,
        filter_query="type=\"{}\"".format(Category.Type.PUBLIC),  # TODO need for release, after front create_table use
                                                                  # - request.filter,
    ).to_api(True)


@api_handler_without_auth(
    "GET",
    "/categories/osProducts",
    params_model=BasePublicListingRequest,
    response_model=CategoryList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_os_products_categories(request: BasePublicListingRequest, request_context) -> CategoryList:
    return lib.Category.rpc_list(
        request,
        filter_query="type=\"{}\"".format(Category.Type.PUBLIC),
    ).to_api(True)


@api_handler_without_auth(
    "GET",
    "/categories/isvs",
    params_model=BasePublicListingRequest,
    response_model=CategoryList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_isv_categories(request: BasePublicListingRequest, request_context) -> CategoryList:
    return lib.Category.rpc_list(
        request,
        filter_query="type=\"{}\"".format(Category.Type.ISV),
    ).to_api(True)


@api_handler_without_auth(
    "GET",
    "/categories/vars",
    params_model=BasePublicListingRequest,
    response_model=CategoryList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_var_categories(request: BasePublicListingRequest, request_context) -> CategoryList:
    return lib.Category.rpc_list(
        request,
        filter_query="type=\"{}\"".format(Category.Type.VAR),
    ).to_api(True)


@api_handler_without_auth(
    "GET",
    "/categories/publishers",
    params_model=BasePublicListingRequest,
    response_model=CategoryList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_publisher_categories(request: BasePublicListingRequest, request_context) -> CategoryList:
    return lib.Category.rpc_list(
        request,
        filter_query="type=\"{}\"".format(Category.Type.PUBLISHER),
    ).to_api(True)


@api_handler_without_auth(
    "GET",
    "/categories/<category_id>",
    context_params=["category_id"],
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_category(category_id, request_context):
    """Return category by Id"""
    return lib.Category.rpc_get_noauth(category_id).to_public().to_api(True)
