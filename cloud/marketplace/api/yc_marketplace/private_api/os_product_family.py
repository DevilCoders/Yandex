"""OS product family"""

from yc_common import logging
from cloud.marketplace.api.yc_marketplace.private_api import private_api_handler
from cloud.marketplace.api.yc_marketplace.utils.i18n import i18n_traverse
from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyCreateRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyDeprecationRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyList
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyOperation
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyResponse
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilySearchRequest
from cloud.marketplace.common.yc_marketplace_common.models.os_product_family import OsProductFamilyUpdateRequest
from cloud.marketplace.common.yc_marketplace_common.utils import errors

log = logging.get_logger(__name__)


@private_api_handler(
    "GET",
    "/osProductFamilies/<product_family_id>",
    context_params=["product_family_id"],
    response_model=OsProductFamilyResponse,
)
@i18n_traverse()
def get_product_family(product_family_id, request_context):
    """Return os product family by Id"""
    family = lib.OsProductFamily.rpc_get_full(product_family_id)
    return family.to_api(True)


@private_api_handler(
    "GET",
    "/osProductFamilies",
    params_model=OsProductFamilySearchRequest,
    response_model=OsProductFamilyList,
)
@i18n_traverse()
def list_product_families(request: OsProductFamilySearchRequest, request_context) -> OsProductFamilyList:
    """List os product families"""
    return lib.OsProductFamily.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        order_by=request.order_by,
        filter=request.filter,
    ).to_api(True)


@private_api_handler(
    "POST",
    "/osProductFamilies",
    model=OsProductFamilyCreateRequest,
    response_model=OsProductFamilyOperation,
)
def create_product_family(request: OsProductFamilyCreateRequest, request_context) -> OsProductFamilyOperation:
    product = lib.OsProduct.rpc_get(request.os_product_id)
    if product.billing_account_id != request.billing_account_id:
        raise errors.OsProductIdError()
    family_create_op = lib.OsProductFamily.rpc_create(request)

    return family_create_op


@private_api_handler(
    "PATCH",
    "/osProductFamilies/<os_product_family_id>",
    model=OsProductFamilyUpdateRequest,
    response_model=OsProductFamilyOperation,
    query_variables=True,
)
def update_product_family(request: OsProductFamilyUpdateRequest, request_context) -> OsProductFamilyOperation:
    return lib.OsProductFamily.rpc_update(request)


@private_api_handler(
    "POST",
    "/osProductFamilies/<os_product_family_id>:deprecate",
    model=OsProductFamilyDeprecationRequest,
    response_model=OsProductFamilyOperation,
    query_variables=True,
)
def deprecate_product_family(request: OsProductFamilyDeprecationRequest, request_context) -> OsProductFamilyOperation:
    return lib.OsProductFamily.rpc_deprecate(request)
