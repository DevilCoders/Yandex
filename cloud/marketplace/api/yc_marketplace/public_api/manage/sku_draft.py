from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authorize_action
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftResponse
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import CreateSkuDraftRequest
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftList
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftOperation


@api_handler(
    "GET",
    "/manage/skuDrafts/<sku_draft_id>",
    context_params=["sku_draft_id"],
    response_model=SkuDraftResponse,
)
@i18n_traverse()
def get_sku_draft(sku_draft_id, request_context) -> SkuDraftResponse:
    draft = lib.SkuDraft.rpc_get(sku_draft_id)
    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_SKU,
        request_context=request_context,
        ba_id=draft.billing_account_id)

    return draft.to_api(True)


@api_handler(
    "GET",
    "/manage/skuDrafts",
    params_model=SkuDraftListingRequest,
    response_model=SkuDraftList,
)
@authz(action_name=ActionNames.LIST_SKUS)
@i18n_traverse()
def list_sku_drafts(request: SkuDraftListingRequest, request_context) -> SkuDraftList:
    drafts = lib.SkuDraft.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        filter_query=request.filter,
        order_by=request.order_by
    )
    return drafts.to_api(True)


@api_handler(
    "POST",
    "/manage/skuDrafts",
    model=CreateSkuDraftRequest,
    response_model=SkuDraftOperation,
)
@authz(action_name=ActionNames.CREATE_SKU)
@i18n_traverse()
def create_sku_draft(request: CreateSkuDraftRequest, request_context) -> SkuDraftOperation:
    return lib.SkuDraft.rpc_create(request)
