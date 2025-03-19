"""Product Image"""

from yc_common import logging
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib

log = logging.get_logger(__name__)


@api_handler_without_auth(
    "GET",
    "/avatar/<avatar_id>",
    context_params=["avatar_id"],
)
@i18n_traverse()
def get_avatar(avatar_id, request_context):
    """Return avatar data by Id"""
    return lib.Avatar.get_image(avatar_id).to_api(True)
