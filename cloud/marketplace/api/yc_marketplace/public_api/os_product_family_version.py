"""Marketplace product versions"""

from cloud.marketplace.api.yc_marketplace.public_api import api_handler_without_auth
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersion
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionLogoIdsRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionResponse
from cloud.marketplace.common.yc_marketplace_common.utils.errors import OsProductFamilyVersionIdError


@api_handler_without_auth(
    "GET",
    "/osProductFamilyVersions/batchLogoUriResolve",
    params_model=OsProductFamilyVersionLogoIdsRequest,
)
@i18n_traverse()
def batch_logo_uri_resolve(request, request_context):
    ids = [i.strip() for i in request.ids.split(",")]
    return lib.OsProductFamilyVersion.rpc_get_batch_logo_uri(ids)


@api_handler_without_auth(
    "GET",
    "/osProductFamilyVersions/<version_id>",
    context_params=["version_id"],
    response_model=OsProductFamilyVersionResponse,
)
@i18n_traverse()
def get_version(version_id, request_context):
    version = lib.OsProductFamilyVersion.rpc_get(version_id)

    if version.status not in OsProductFamilyVersion.Status.PUBLIC:
        raise OsProductFamilyVersionIdError()

    return version.to_api(True)
