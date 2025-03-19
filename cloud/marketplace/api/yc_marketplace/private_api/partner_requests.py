from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib import PartnerRequest
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequestPublicList
from cloud.marketplace.common.yc_marketplace_common.models.partner_requests import PartnerRequestsListingRequest


@private_api_handler(
    "POST",
    "/partnerRequests/<partner_request_id>:approve",
    context_params=["partner_request_id"],
    response_model=OperationV1Beta1,
    query_variables=True,
)
@i18n_traverse()
def accept_partner_request(partner_request_id, request_context):
    return PartnerRequest.rpc_approve(partner_request_id).to_api(False)


@private_api_handler(
    "POST",
    "/partnerRequests/<partner_request_id>:decline",
    context_params=["partner_request_id"],
    response_model=OperationV1Beta1,
    query_variables=True,
)
@i18n_traverse()
def decline_partner_request(partner_request_id, request_context):
    return PartnerRequest.rpc_decline(partner_request_id).to_api(False)


@private_api_handler(
    "GET",
    "/partnerRequests",
    params_model=PartnerRequestsListingRequest,
    response_model=PartnerRequestPublicList,
)
@i18n_traverse()
def list_partner_request(request: PartnerRequestsListingRequest, request_context):
    return PartnerRequest.rpc_list(request, order_by=None, filter=request.filter).to_api(False)
