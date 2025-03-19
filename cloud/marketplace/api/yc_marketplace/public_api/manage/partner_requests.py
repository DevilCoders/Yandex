from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authz_by_object
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib import PartnerRequest


@api_handler(
    "POST",
    "/manage/partner_requests/<request_id>:close",
    context_params=["request_id"],
    response_model=OperationV1Beta1,
    query_variables=True,
)
@i18n_traverse()
def close_partner_request(request_context):
    auth_func = api_authz_by_object(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.UPDATE_PUBLISHER,
        request_context=request_context,
        auth_fields={"ba_id": "billing_account_id"},
    )
    return PartnerRequest.rpc_close(auth_func).to_api(False)
