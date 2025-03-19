from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.form import Form
from cloud.marketplace.common.yc_marketplace_common.models.form import FormResponse


@api_handler_without_auth("GET", "/form/<form_id>",
                          context_params=["form_id"],
                          response_model=FormResponse)
@i18n_traverse()
def public_get_form(form_id, request_context):
    return Form.rpc_get(form_id).to_api(True)
