"""Product Image"""

from yc_common import logging
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib

log = logging.get_logger(__name__)


@api_handler_without_auth(
    "GET",
    "/eula/<eula_id>",
    context_params=["eula_id"],
)
@i18n_traverse()
def get_eula(eula_id, request_context):
    """Return eula data by Id"""
    return lib.Eula.get_eula(eula_id).to_api(True)
