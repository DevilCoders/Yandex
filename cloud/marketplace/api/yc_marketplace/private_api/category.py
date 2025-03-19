"""Category"""
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryAddResourcesRequest
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryOperation
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryRemoveResourcesRequest
from cloud.marketplace.common.yc_marketplace_common.models.category import CategoryUpdateRequest


@private_api_handler(
    "POST",
    "/categories",
    model=CategoryCreateRequest,
    response_model=CategoryOperation,
)
@i18n_traverse()
def create_category(request: CategoryCreateRequest, request_context) -> CategoryOperation:
    return lib.Category.rpc_create(request)


@private_api_handler(
    "PATCH",
    "/categories/<category_id>",
    model=CategoryUpdateRequest,
    response_model=CategoryOperation,
    query_variables=True,
)
@i18n_traverse()
def update_category(request: CategoryUpdateRequest, request_context) -> CategoryOperation:
    return lib.Category.rpc_update(request)


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/categories/<category_id>:addResources",
    model=CategoryAddResourcesRequest,
    response_model=CategoryOperation,
    query_variables=True,
)
@i18n_traverse()
def add_to_category(request: CategoryAddResourcesRequest, request_context):
    return lib.Ordering.update_priority(request.category_id, request.resource_ids)


# TODO add to spec after design admin panel
@private_api_handler(
    "POST",
    "/categories/<category_id>:removeResources",
    model=CategoryRemoveResourcesRequest,
    response_model=CategoryOperation,
    query_variables=True,
)
@i18n_traverse()
def remove_from_category(request: CategoryRemoveResourcesRequest, request_context):
    return lib.Ordering.remove(request.category_id, request.resource_ids)
