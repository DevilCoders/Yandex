from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftList
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftOperation
from cloud.marketplace.common.yc_marketplace_common.models.base import BaseMktListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.sku_draft import SkuDraftResponse


@private_api_handler(
    "GET",
    "/skuDrafts/<sku_draft_id>",
    context_params=["sku_draft_id"],
    response_model=SkuDraftResponse,
)
@i18n_traverse()
def get_sku_draft(sku_draft_id, request_context) -> SkuDraftResponse:
    draft = lib.SkuDraft.rpc_get(sku_draft_id)
    return draft.to_api(True)


@private_api_handler(
    "GET",
    "/skuDrafts",
    params_model=BaseMktListingRequest,
    response_model=SkuDraftList,
)
@i18n_traverse()
def list_sku_drafts(request: BaseMktListingRequest, request_context) -> SkuDraftList:
    drafts = lib.SkuDraft.rpc_list(
        request,
        filter_query=request.filter,
        order_by=request.order_by
    )
    return drafts.to_api(True)


@private_api_handler(
    "POST",
    "/skuDrafts/<sku_draft_id>:accept",
    context_params=["sku_draft_id"],
    response_model=SkuDraftOperation,
    query_variables=True,
)
@i18n_traverse()
def accept_sku_draft(sku_draft_id, request_context) -> SkuDraftOperation:
    op = lib.SkuDraft.rpc_accept(
        sku_draft_id,
    )
    return op


@private_api_handler(
    "POST",
    "/skuDrafts/<sku_draft_id>:reject",
    context_params=["sku_draft_id"],
    response_model=SkuDraftOperation,
    query_variables=True,
)
@i18n_traverse()
def reject_sku_draft(sku_draft_id, request_context) -> SkuDraftOperation:
    op = lib.SkuDraft.rpc_reject(
        sku_draft_id
    )
    return op
