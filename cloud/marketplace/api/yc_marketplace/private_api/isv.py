from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.isv import Isv
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvOperation
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvRequest
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvResponse
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvUpdateStatusRequest


@private_api_handler(
    "POST",
    "/isvs/<isv_id>:setStatus",
    context_params=["isv_id"],
    model=IsvUpdateStatusRequest,
    response_model=IsvOperation,
    query_variables=True,
)
@i18n_traverse()
def update_isv_status(request, request_context):
    return Isv.rpc_set_status(request).to_api(False)


@private_api_handler(
    "POST",
    "/isvs",
    model=IsvRequest,
    response_model=IsvOperation,
)
@i18n_traverse()
def create_isv(request, request_context) -> IsvOperation:
    return Isv.rpc_create(request, request_context, admin=True).to_api(False)


@private_api_handler(
    "GET",
    "/isvs/<ba_id>:byBillingAccountId",
    context_params=["ba_id"],
    response_model=IsvResponse,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_isv_by_ba(ba_id, request_context) -> IsvResponse:
    return Isv.rpc_get_by_ba(ba_id).to_api(True)
