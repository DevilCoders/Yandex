from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common.lib.form import Form
from cloud.marketplace.common.yc_marketplace_common.models.form import FormCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.form import FormList
from cloud.marketplace.common.yc_marketplace_common.models.form import FormListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.form import FormResponse
from cloud.marketplace.common.yc_marketplace_common.models.form import FormUpdateRequest


@private_api_handler(
    "POST",
    "/form/<form_id>",
    response_model=OperationV1Beta1,
    model=FormUpdateRequest,
    query_variables=True,
)
@i18n_traverse()
def update_form(request: FormUpdateRequest, request_context):
    return Form.rpc_update(request).to_api(False)


@private_api_handler(
    "POST",
    "/form",
    response_model=OperationV1Beta1,
    model=FormCreateRequest,
)
@i18n_traverse()
def create_form(request: FormCreateRequest, request_context):
    return Form.rpc_create(request).to_api(False)


@private_api_handler(
    "GET",
    "/form",
    params_model=FormListingRequest,
    response_model=FormList)
@i18n_traverse()
def list_forms(request: FormList, **kwargs):
    forms = Form.rpc_list(request, filter_query=request.filter, order_by=request.order_by)

    return forms.to_api(False)


@private_api_handler("GET", "/form/<form_id>",
                     context_params=["form_id"],
                     response_model=FormResponse)
@i18n_traverse()
def get_form(form_id, request_context):
    return Form.rpc_get(form_id).to_api(False)
