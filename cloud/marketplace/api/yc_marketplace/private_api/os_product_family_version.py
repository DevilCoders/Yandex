"""Marketplace product version private methods"""
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import OsProductFamilyVersionList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionPrivateListRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionSetImageIdRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import \
    OsProductFamilyVersionSetStatusRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family_version import PublishRequest


@private_api_handler(
    "POST",
    "/osProductFamilyVersions/<os_product_family_version_id>:setStatus",
    model=OsProductFamilyVersionSetStatusRequest,
    response_model=OsProductFamilyVersionOperation,
    query_variables=True,
)
@i18n_traverse()
def set_status_product_family_version(request: OsProductFamilyVersionSetStatusRequest, request_context):
    """Set status of product family version"""
    return lib.OsProductFamilyVersion.rpc_force_update(request.os_product_family_version_id, {
        "status": request.status,
    })


@private_api_handler(
    "POST",
    "/osProductFamilyVersions/<os_product_family_version_id>:setImageId",
    model=OsProductFamilyVersionSetImageIdRequest,
    response_model=OsProductFamilyVersionOperation,
    query_variables=True,
)
@i18n_traverse()
def set_image_id_product_family_version(request: OsProductFamilyVersionSetImageIdRequest, request_context):
    """Set image id of product family version"""
    return lib.OsProductFamilyVersion.rpc_force_update(request.os_product_family_version_id, {
        "image_id": request.image_id,
    })


@private_api_handler(
    "POST",
    "/osProductFamilyVersions/<os_product_family_version_id>:publish",
    model=PublishRequest,
    response_model=OsProductFamilyVersionOperation,
    query_variables=True,
)
@i18n_traverse()
def publish_product_family_version(request: PublishRequest, request_context):
    """Publish product family version"""
    return lib.OsProductFamilyVersion.rpc_publish(request.os_product_family_version_id, request.pool_size)


@private_api_handler(
    "GET",
    "/osProductFamilyVersions",
    params_model=OsProductFamilyVersionPrivateListRequest,
    response_model=OsProductFamilyVersionList,
)
@i18n_traverse()
def get_product_family_versions(request: OsProductFamilyVersionPrivateListRequest, request_context):
    """Search marketplace product versions by publisher"""
    return lib.OsProductFamilyVersion.rpc_list(request, filter=request.filter, order_by=request.order_by).to_api(True)
