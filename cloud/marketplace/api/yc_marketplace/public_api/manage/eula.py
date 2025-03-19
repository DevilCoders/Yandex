from flask import Request
from flask import request as flask_request

from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.eula import EulaResponse


def parse_request(request: Request, type_options, body):
    return request.form.to_dict()


@api_handler(
    "POST",
    "/manage/eulas",
    response_model=EulaResponse,
    process_request_rules={
        "multipart/form-data": parse_request,
    },
)
@i18n_traverse()
def create_image(request_context) -> EulaResponse:
    return lib.Eula.rpc_create(flask_request.files)
