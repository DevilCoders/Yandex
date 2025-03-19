from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildFinishRequest
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildList
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildOperation
from cloud.marketplace.common.yc_marketplace_common.models.build import BuildResponse


@private_api_handler(
    "GET",
    "/builds/<build_id>",
    context_params=["build_id"],
    response_model=BuildResponse,
)
def get_build(build_id, request_context) -> BuildResponse:
    build = lib.Build.rpc_get(build_id)
    return build.to_api(False)


@private_api_handler(
    "GET",
    "/builds",
    params_model=BaseMktListingRequest,
    response_model=BuildList,
)
def list_builds(request: BaseMktListingRequest, request_context) -> BuildList:
    builds = lib.Build.rpc_list(
        request,
        filter_query=request.filter,
        order_by=request.order_by
    )
    return builds.to_api(True)


@private_api_handler(
    "POST",
    "/builds/<build_id>:start",
    context_params=["build_id"],
    response_model=BuildOperation,
    query_variables=True,
)
def start_build(build_id, request_context) -> BuildOperation:
    return lib.Build.rpc_start(build_id).to_api(False)


@private_api_handler(
    "POST",
    "/builds/<build_id>:finish",
    model=BuildFinishRequest,
    response_model=BuildOperation,
    query_variables=True,
)
def finish_build(request: BuildFinishRequest, request_context) -> BuildOperation:
    request.validate()
    return lib.Build.rpc_finish(request.build_id, request.compute_image_id, request.status)
