"""Isv API"""
from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authz_by_object
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.isv import Isv
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktManageListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvList
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvOperation
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvRequest
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvResponse
from cloud.marketplace.common.yc_marketplace_common.models.isv import IsvUpdateRequest


@api_handler(
    "GET",
    "/manage/isvs/<isv_id>",
    context_params=["isv_id"],
    response_model=IsvResponse,
)
@i18n_traverse()
def get_isv(isv_id, request_context) -> IsvResponse:
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_ISV,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )

    isv = Isv.rpc_get(isv_id, auth=auth_func)

    return isv.to_api(True)


@api_handler(
    "GET",
    "/manage/isvs",
    params_model=BaseMktManageListingRequest,
    response_model=IsvList,
)
@authz(action_name=ActionNames.LIST_ISVS)
@i18n_traverse()
def list_isvs(request: BaseMktManageListingRequest, request_context) -> IsvList:
    return Isv.rpc_list(request, billing_account_id=request.billing_account_id, filter_query=request.filter,
                        order_by=request.order_by).to_api(True)


@api_handler(
    "POST",
    "/manage/isvs",
    model=IsvRequest,
    response_model=OperationV1Beta1,
)
@authz(action_name=ActionNames.CREATE_ISV)
@i18n_traverse()
def create_isv(request, request_context) -> IsvOperation:
    return Isv.rpc_create(request, request_context).to_api(False)


@api_handler(
    "PATCH",
    "/manage/isvs/<isv_id>",
    model=IsvUpdateRequest,
    response_model=OperationV1Beta1,
    context_params=["isv_id"],
    query_variables=True,
)
@i18n_traverse()
def update_isv(request, request_context) -> IsvOperation:
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.UPDATE_ISV,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )

    return Isv.rpc_update(request, request_context, auth=auth_func).to_api(False)
