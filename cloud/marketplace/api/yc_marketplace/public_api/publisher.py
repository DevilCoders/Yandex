"""Publisher API"""

from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.publisher import Publisher
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherList
from cloud.marketplace.common.yc_marketplace_common.models.publisher import PublisherResponse


@api_handler_without_auth(
    "GET",
    "/publishers/<publisher_id>",
    context_params=["publisher_id"],
    response_model=PublisherResponse,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_public_publisher(publisher_id, request_context) -> PublisherResponse:
    return Publisher.rpc_get_public(publisher_id).to_api(True)


@api_handler_without_auth(
    "GET",
    "/publishers",
    params_model=BaseMktListingRequest,
    response_model=PublisherList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_public_publishers(request: BaseMktListingRequest, request_context) -> PublisherList:
    return Publisher.rpc_public_list(request, filter_query=request.filter, order_by=request.order_by).to_api(True)
