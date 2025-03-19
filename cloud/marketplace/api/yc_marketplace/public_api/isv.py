"""Isv API"""

from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.isv import Isv
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvList
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvResponse


# get ISV by ID
@api_handler_without_auth(
    "GET",
    "/isvs/<isv_id>",
    context_params=["isv_id"],
    response_model=IsvResponse,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_public_isv(isv_id, request_context) -> IsvResponse:
    return Isv.rpc_get_public(isv_id).to_api(True)


@api_handler_without_auth(
    "GET",
    "/isvs",
    params_model=BaseMktListingRequest,
    response_model=IsvList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_public_isvs(request: BaseMktListingRequest, request_context) -> IsvList:
    return Isv.rpc_public_list(request, filter_query=request.filter, order_by=request.order_by).to_api(True)
