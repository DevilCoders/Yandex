"""OS product family"""

from yc_common import logging
from cloud.marketplace.api.yc_marketplace.public_api import api_handler
from cloud.marketplace.api.yc_marketplace.utils.authorization import ActionNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import ServiceNames
from cloud.marketplace.api.yc_marketplace.utils.authorization import api_authorize_action
from cloud.marketplace.api.yc_marketplace.utils.authorization import authz
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


@api_handler(
    "GET",
    "/manage/osProductFamilies/<product_family_id>",
    context_params=["product_family_id"],
    response_model=OsProductFamilyResponse,
)
@i18n_traverse()
def get_product_family(product_family_id, request_context):
    """Return os product family by Id"""

    family = lib.OsProductFamily.rpc_get_full(product_family_id)

    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.GET_OS_PRODUCT_FAMILY,
        request_context=request_context,
        ba_id=family.billing_account_id)

    return family.to_api(True)


@api_handler(
    "GET",
    "/manage/osProductFamilies",
    params_model=OsProductFamilySearchRequest,
    response_model=OsProductFamilyList,
)
@authz(action_name=ActionNames.LIST_OS_PRODUCT_FAMILIES)
@i18n_traverse()
def list_product_families(request: OsProductFamilySearchRequest, request_context) -> OsProductFamilyList:
    """List os product families"""
    return lib.OsProductFamily.rpc_list(
        request,
        billing_account_id=request.billing_account_id,
        order_by=request.order_by,
        filter=request.filter,
    ).to_api(True)


@api_handler(
    "POST",
    "/manage/osProductFamilies",
    model=OsProductFamilyCreateRequest,
    response_model=OsProductFamilyOperation,
)
@authz(action_name=ActionNames.CREATE_OS_PRODUCT)
@i18n_traverse()
def create_product_family(request: OsProductFamilyCreateRequest, request_context) -> OsProductFamilyOperation:
    product = lib.OsProduct.rpc_get(request.os_product_id)
    if product.billing_account_id != request.billing_account_id:
        raise errors.OsProductIdError()
    family_create_op = lib.OsProductFamily.rpc_create(request)

    return family_create_op


@api_handler(
    "PATCH",
    "/manage/osProductFamilies/<os_product_family_id>",
    model=OsProductFamilyUpdateRequest,
    response_model=OsProductFamilyOperation,
    query_variables=True,
)
@i18n_traverse()
def update_product_family(request: OsProductFamilyUpdateRequest, request_context) -> OsProductFamilyOperation:
    family = lib.OsProductFamily.get(request.os_product_family_id)

    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.UPDATE_OS_PRODUCT_FAMILY,
        request_context=request_context,
        ba_id=family.billing_account_id)

    return lib.OsProductFamily.rpc_update(request)


@api_handler(
    "POST",
    "/manage/osProductFamilies/<os_product_family_id>:deprecate",
    model=OsProductFamilyDeprecationRequest,
    response_model=OsProductFamilyOperation,
    query_variables=True,
)
@i18n_traverse()
def deprecate_product_family(request: OsProductFamilyDeprecationRequest, request_context) -> OsProductFamilyOperation:
    family = lib.OsProductFamily.get(request.os_product_family_id)

    api_authorize_action(
        service_name=ServiceNames.PUBLIC,
        action_name=ActionNames.DEPRECATE_OS_PRODUCT_FAMILY,
        request_context=request_context,
        ba_id=family.billing_account_id)
    return lib.OsProductFamily.rpc_deprecate(request)
