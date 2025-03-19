"""Var API"""

from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.var import Var
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.var import VarList
from cloud.marketplace.common.yc_marketplace_common.models.var import VarResponse


@api_handler_without_auth(
    "GET",
    "/vars/<var_id>",
    context_params=["var_id"],
    response_model=VarResponse,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_public_var(var_id, request_context) -> VarResponse:
    return Var.rpc_get_public(var_id).to_api(True)


@api_handler_without_auth(
    "GET",
    "/vars",
    params_model=BaseMktListingRequest,
    response_model=VarList,
)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def list_public_vars(request: BaseMktListingRequest, request_context) -> VarList:
    return Var.rpc_public_list(request, filter_query=request.filter, order_by=request.order_by).to_api(True)
