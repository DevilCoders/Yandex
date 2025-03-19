from flask import Blueprint

from yc_common import logging
from yc_common.api.handling import generic_api_handler
from yc_common.api.middlewares.flask import FlaskMiddleware

log = logging.get_logger(__name__)

private_api_blueprint = Blueprint("private-api", __name__, url_prefix="/marketplace/v1alpha1/private")
http_middleware = FlaskMiddleware(private_api_blueprint)


def private_api_handler(*args, **kwargs):
    return generic_api_handler(
        http_middleware,
        *args,
        **kwargs,
    )


import cloud.marketplace.api.yc_marketplace.private_api.avatar  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.blueprint  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.build  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.category  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.eula  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.form  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.health  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.i18n  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.isv  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.os_product  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.os_product_family  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.os_product_family_version  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.partner_requests  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.product_slug  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.publisher  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.saas_product  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.simple_product  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.sku_draft  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.task  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.telemetry  # noqa
import cloud.marketplace.api.yc_marketplace.private_api.var  # noqa
