"""Marketplace product"""

from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersionList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersionListRequest


@api_handler(
    "GET",
    "/manage/osProductFamilyVersions",
    params_model=OsProductFamilyVersionListRequest,
    response_model=OsProductFamilyVersionList,
)
@authz(action_name=ActionNames.LIST_OS_PRODUCT_FAMILY_VERSIONS)
@i18n_traverse()
def get_product_family_versions(request, request_context):
    """Search marketplace product versions by publisher"""
    return lib.OsProductFamilyVersion.rpc_public_list(request, filter=request.filter,
                                                      billing_account_id=request.billing_account_id).to_api(True)
