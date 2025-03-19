"""OS product family"""

from yc_common import logging
from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authorize_action
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamily as OsProductFamilyScheme
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyListingRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyResponse

log = logging.get_logger(__name__)


@api_handler_without_auth(
    "GET",
    "/osProductFamilies/<product_family_id>",
    context_params=["product_family_id"],
    response_model=OsProductFamilyResponse,
)
@i18n_traverse()
def get_public_product_family(product_family_id, request_context):
    """Return os product family by Id"""
    family = lib.OsProductFamily.rpc_get_full(product_family_id)
    if family.status not in OsProductFamilyScheme.Status.PUBLIC:
        api_authorize_action(
            service_name=ServiceNames.PUBLIC,
            action_name=ActionNames.GET_OS_PRODUCT_FAMILY,
            request_context=request_context,
            ba_id=family.billing_account_id)
    return family.to_api(True)


@api_handler_without_auth(
    "GET",
    "/osProductFamilies",
    params_model=OsProductFamilyListingRequest,
    response_model=OsProductFamilyList,
)
@i18n_traverse()
def list_public_product_families(request: OsProductFamilyListingRequest, request_context) -> OsProductFamilyList:
    """List os product families"""
    return lib.OsProductFamily.rpc_list_public(request, order_by=request.order_by, filter=request.filter).to_api(True)
