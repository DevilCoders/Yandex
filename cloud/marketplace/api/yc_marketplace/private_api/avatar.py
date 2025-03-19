"""Product Image"""

from flask import Request
from flask import request as flask_request

from yc_common import logging
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.avatar import AvatarResponse

log = logging.get_logger(__name__)


def parse_image_request(request: Request, type_options, body):
    return request.form.to_dict()


@private_api_handler(
    "POST",
    "/avatars",
    response_model=AvatarResponse,
    process_request_rules={
        "multipart/form-data": parse_image_request,
    },
)
@i18n_traverse()
def create_image(request_context) -> AvatarResponse:
    return lib.Avatar.rpc_create(flask_request.files)
