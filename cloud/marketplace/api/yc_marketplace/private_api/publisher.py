from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.publisher import Publisher
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherList
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherOperation
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherRequest
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherResponse
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherUpdateStatusRequest


@private_api_handler(
    "GET",
    "/publishers/<publisher_id>",
    context_params=["publisher_id"],
    response_model=PublisherResponse,
)
@i18n_traverse()
def get_public_publisher(publisher_id, request_context) -> PublisherResponse:
    return Publisher.rpc_get(publisher_id).to_api(True)


@private_api_handler(
    "GET",
    "/publishers",
    params_model=BaseMktListingRequest,
    response_model=PublisherList,
)
@i18n_traverse()
def list_public_publishers(request: BaseMktListingRequest, request_context) -> PublisherList:
    return Publisher.rpc_list(request, filter_query=request.filter, order_by=request.order_by).to_api(True)


@private_api_handler(
    "POST",
    "/publishers/<publisher_id>:setStatus",
    context_params=["publisher_id"],
    model=PublisherUpdateStatusRequest,
    response_model=PublisherOperation,
    query_variables=True,
)
@i18n_traverse()
def update_publisher_status(request, request_context):
    return Publisher.rpc_set_status(request).to_api(False)


@private_api_handler(
    "POST",
    "/publishers",
    model=PublisherRequest,
    response_model=PublisherOperation,
)
@i18n_traverse()
def create_publisher(request, request_context) -> PublisherOperation:
    return Publisher.rpc_create(request, request_context, admin=False).to_api(False)
