import functools

import yc_as_client

from yc_common.api.request_context import RequestContext
from yc_common.clients import access_service
from yc_common.constants import ServiceNames as CommonServiceNames


class ServiceNames:
    PUBLIC = CommonServiceNames.MARKETPLACE


class ActionNames:
    # avatar
    UPLOAD_AVATAR = "avatar.create"

    # os product
    CREATE_OS_PRODUCT = "osProducts.create"
    UPDATE_OS_PRODUCT = "osProducts.update"
    LIST_OS_PRODUCTS = "osProducts.list"
    GET_OS_PRODUCT = "osProducts.get"

    # os product
    CREATE_SKU = "skus.create"
    UPDATE_SKU = "skus.update"
    LIST_SKUS = "skus.list"
    GET_SKU = "skus.get"

    # os product family
    LIST_OS_PRODUCT_FAMILIES = "osProductFamilies.list"
    GET_OS_PRODUCT_FAMILY = "osProductFamilies.get"
    UPDATE_OS_PRODUCT_FAMILY = "osProductFamilies.update"
    DEPRECATE_OS_PRODUCT_FAMILY = "osProductFamilies.deprecate"

    # os product family version
    LIST_OS_PRODUCT_FAMILY_VERSIONS = "osProductFamilyVersions.list"

    # all products
    LIST_PRODUCTS = "products.list"

    # publisher
    UPDATE_PUBLISHER = "publishers.update"
    LIST_PUBLISHERS = "publishers.list"
    GET_PUBLISHER = "publishers.get"
    CREATE_PUBLISHER = "publishers.create"

    # publisher
    UPDATE_ISV = "isvs.update"
    LIST_ISVS = "isvs.list"
    GET_ISV = "isvs.get"
    CREATE_ISV = "isvs.create"

    # publisher
    UPDATE_VAR = "vars.update"
    LIST_VARS = "vars.list"
    GET_VAR = "vars.get"
    CREATE_VAR = "vars.create"


def get_resource(ba_id=None, **kwargs):
    if ba_id is not None:
        resource = yc_as_client.entities.Resource(
            id=ba_id,
            type="billing.account",
        )
    else:
        raise ValueError("No resource supplied.")

    return resource


def api_authorize_action(*, service_name: str, action_name: str, request_context: RequestContext, ba_id=None):
    access_service.AccessService.instance().authorize(
        iam_token=request_context.auth.token_bytes,
        subject=None,
        signature=None,
        permission="{}.{}".format(service_name, action_name),
        resource_path=[get_resource(ba_id)],
        request_id=request_context.request_id,
    )


def api_authz_by_object(*, service_name: str, action_name: str, request_context: RequestContext, auth_fields: dict):
    def inner(obj):
        kwargs = {f: getattr(obj, auth_fields[f]) for f in auth_fields}
        access_service.AccessService.instance().authorize(
            iam_token=request_context.auth.token_bytes,
            subject=None,
            signature=None,
            permission="{}.{}".format(service_name, action_name),
            resource_path=[get_resource(**kwargs)],
            request_id=request_context.request_id,
        )

    return inner


def ba_extract_from_req(request, *args, **kwargs):
    return request.billing_account_id


def authz(*, action_name, service_name=ServiceNames.PUBLIC, ba_id_extractor=ba_extract_from_req):
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            token = kwargs["request_context"].auth.token_bytes
            request_id = kwargs["request_context"].request_id

            ba_id = ba_id_extractor(*args, **kwargs) if callable(ba_id_extractor) else None

            access_service.AccessService.instance().authorize(
                iam_token=token,
                subject=None,
                signature=None,
                permission="{}.{}".format(service_name, action_name),
                resource_path=[get_resource(ba_id)],
                request_id=request_id,
            )

            return func(*args, **kwargs)

        return wrapper

    return decorator
