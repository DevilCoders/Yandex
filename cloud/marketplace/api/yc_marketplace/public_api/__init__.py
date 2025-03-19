from flask import Blueprint

from yc_common import logging
from yc_common.api.authentication import TokenAuthWithSignVerification
from yc_common.api.handling import generic_api_handler
from yc_common.api.middlewares.flask import FlaskMiddleware
from yc_common.constants import ServiceNames

log = logging.get_logger(__name__)

public_api_blueprint = Blueprint("public-api", __name__, url_prefix="/marketplace/v1alpha1/console")
http_middleware = FlaskMiddleware(public_api_blueprint)


def api_handler(*args, **kwargs):
    return generic_api_handler(
        http_middleware, *args,
        auth_method=TokenAuthWithSignVerification(ServiceNames.MARKETPLACE),
        doc_writer=None, **kwargs,
    )


def api_handler_without_auth(*args, **kwargs):
    return generic_api_handler(
        http_middleware, *args,
        auth_method=None,
        doc_writer=None, **kwargs,
    )


import cloud.marketplace.api.yc_marketplace.public_api.avatar  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.category  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.eula  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.form  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.isv  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.avatar  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.isv  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.os_product  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.os_product_family  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.os_product_family_version  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.partner_requests  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.publisher  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.saas_product  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.simple_product  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.sku  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.sku_draft  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.task  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.manage.var  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.metrics  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.os_product  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.os_product_family  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.os_product_family_version  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.publisher  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.saas_product  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.simple_product  # noqa
import cloud.marketplace.api.yc_marketplace.public_api.var  # noqa
