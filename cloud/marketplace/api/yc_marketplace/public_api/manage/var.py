"""Var API"""

from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authz_by_object
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.var import Var
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.var import VarList
from cloud.marketplace.common.yc_marketplace_common.models.var import VarOperation
from cloud.marketplace.common.yc_marketplace_common.models.var import VarRequest
from cloud.marketplace.common.yc_marketplace_common.models.var import VarResponse
from cloud.marketplace.common.yc_marketplace_common.models.var import VarUpdateRequest


@api_handler(
    "GET",
    "/manage/vars/<var_id>",
    context_params=["var_id"],
    response_model=VarResponse,
)
@i18n_traverse()
def get_var(var_id, request_context) -> VarResponse:
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_VAR,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )

    var = Var.rpc_get(var_id, auth=auth_func)

    return var.to_api(True)


@api_handler(
    "GET",
    "/manage/vars",
    params_model=BaseMktManageListingRequest,
    response_model=VarList,
)
@authz(action_name=ActionNames.LIST_VARS)
@i18n_traverse()
def list_vars(request: BaseMktManageListingRequest, request_context) -> VarList:
    return Var.rpc_list(request, billing_account_id=request.billing_account_id, filter_query=request.filter,
                        order_by=request.order_by).to_api(True)


@api_handler(
    "POST",
    "/manage/vars",
    model=VarRequest,
    response_model=OperationV1Beta1,
)
@authz(action_name=ActionNames.CREATE_VAR)
@i18n_traverse()
def create_var(request, request_context) -> VarOperation:
    return Var.rpc_create(request, request_context).to_api(False)


@api_handler(
    "PATCH",
    "/manage/vars/<var_id>",
    model=VarUpdateRequest,
    response_model=OperationV1Beta1,
    context_params=["var_id"],
    query_variables=True,
)
@i18n_traverse()
def update_var(request, request_context) -> VarOperation:
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.UPDATE_VAR,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )

    return Var.rpc_update(request, request_context, auth=auth_func).to_api(False)
