from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintList
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintOperation
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintResponse
from cloud.marketplace.common.yc_marketplace_common.models.blueprint import BlueprintUpdateRequest


@private_api_handler(
    "GET",
    "/blueprints/<blueprint_id>",
    context_params=["blueprint_id"],
    response_model=BlueprintResponse,
)
def get_blueprint(blueprint_id, request_context) -> BlueprintResponse:
    blueprint = lib.Blueprint.rpc_get(blueprint_id)
    return blueprint.to_api(False)


@private_api_handler(
    "GET",
    "/blueprints",
    params_model=BaseMktListingRequest,
    response_model=BlueprintList,
)
def list_blueprints(request: BaseMktListingRequest, request_context) -> BlueprintList:
    blueprints = lib.Blueprint.rpc_list(
        request,
        filter_query=request.filter,
        order_by=request.order_by
    )
    return blueprints.to_api(True)


@private_api_handler(
    "POST",
    "/blueprints",
    model=BlueprintCreateRequest,
    response_model=BlueprintOperation,
)
def create_blueprint(request: BlueprintCreateRequest, request_context) -> BlueprintOperation:
    return lib.Blueprint.rpc_create(request).to_api(False)


@private_api_handler(
    "PATCH",
    "/blueprints/<blueprint_id>",
    model=BlueprintUpdateRequest,
    response_model=BlueprintOperation,
    query_variables=True,
)
def update_blueprint(request: BlueprintUpdateRequest, request_context) -> BlueprintOperation:
    return lib.Blueprint.rpc_update(request).to_api(False)


@private_api_handler(
    "POST",
    "/blueprints/<blueprint_id>:accept",
    context_params=["blueprint_id"],
    response_model=BlueprintOperation,
    query_variables=True,
)
def accept_blueprint(blueprint_id, request_context) -> BlueprintOperation:
    return lib.Blueprint.rpc_accept(blueprint_id).to_api(False)


@private_api_handler(
    "POST",
    "/blueprints/<blueprint_id>:build",
    context_params=["blueprint_id"],
    response_model=BlueprintOperation,
    query_variables=True,
)
def build_blueprint(blueprint_id, request_context) -> BlueprintOperation:
    return lib.Blueprint.rpc_build(blueprint_id)


@private_api_handler(
    "POST",
    "/blueprints/<blueprint_id>:reject",
    context_params=["blueprint_id"],
    response_model=BlueprintOperation,
    query_variables=True,
)
def reject_blueprint(blueprint_id, request_context) -> BlueprintOperation:
    return lib.Blueprint.rpc_reject(blueprint_id).to_api(False)
