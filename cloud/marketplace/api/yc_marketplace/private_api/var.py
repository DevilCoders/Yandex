from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib import Var
from cloud.marketplace.common.yc_marketplace_common.models.var import VarOperation
from cloud.marketplace.common.yc_marketplace_common.models.var import VarRequest
from cloud.marketplace.common.yc_marketplace_common.models.var import VarResponse
from cloud.marketplace.common.yc_marketplace_common.models.var import VarUpdateStatusRequest


@private_api_handler(
    "POST",
    "/vars/<var_id>:setStatus",
    context_params=["var_id"],
    model=VarUpdateStatusRequest,
    response_model=VarOperation,
    query_variables=True,
)
@i18n_traverse()
def update_var_status(request, request_context):
    return Var.rpc_set_status(request).to_api(False)


@private_api_handler(
    "POST",
    "/vars",
    model=VarRequest,
    response_model=VarOperation,
)
@i18n_traverse()
def create_var(request, request_context) -> VarOperation:
    return Var.rpc_create(request, request_context, admin=True).to_api(False)


@private_api_handler(
    "GET",
    "/isvs/<ba_id>:byBillingAccountId",
    context_params=["ba_id"],
    response_model=VarResponse,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_var_by_ba(ba_id, request_context) -> VarResponse:
    return Var.rpc_get_by_ba(ba_id).to_api(True)
