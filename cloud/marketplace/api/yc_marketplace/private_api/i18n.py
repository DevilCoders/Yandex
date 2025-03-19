from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.i18n import BulkI18nCreatePublic
from cloud.marketplace.common.yc_marketplace_common.models.i18n import I18nGetRequest


@private_api_handler(
    "POST",
    "/i18n",
    model=BulkI18nCreatePublic,
    response_model=BulkI18nCreatePublic,
)
def create_bulk_i18n(request: BulkI18nCreatePublic, request_context) -> BulkI18nCreatePublic:
    return lib.I18n.rpc_create_bulk(request).to_api(False)


@private_api_handler(
    "GET",
    "/i18n",
    context_params=["id"],
    params_model=I18nGetRequest,
    query_variables=True,
)
@i18n_traverse()
def get_i18n(request: I18nGetRequest, request_context):
    return {"text": request.id}
