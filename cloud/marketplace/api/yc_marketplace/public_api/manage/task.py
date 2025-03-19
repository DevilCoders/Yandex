from yc_common.clients.kikimr import retry_idempotent_kikimr_errors
from yc_common.clients.models.operations import OperationV1Beta1
from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib


@api_handler("GET", "/manage/operations/<operation_id>",
                     context_params=["operation_id"],
                     response_model=OperationV1Beta1)
@i18n_traverse()
@retry_idempotent_kikimr_errors
def get_operation(operation_id, request_context):
    return lib.TaskUtils.get(operation_id).to_operation_v1beta1().to_api(False)
