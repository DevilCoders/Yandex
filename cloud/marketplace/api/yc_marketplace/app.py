import os  # noqa isort:skip

from flask import Flask
# noinspection PyUnresolvedReferences
from yc_common import logging
from yc_common.api.middlewares.flask import FlaskMiddleware

from cloud.marketplace.api.yc_marketplace import config  # noqa isort:skip
from cloud.marketplace.api.yc_marketplace.private_api import private_api_blueprint
from cloud.marketplace.api.yc_marketplace.public_api import public_api_blueprint


def create_app(app_opts=None):
    devel_mode = os.getenv("DEVEL")  # noqa isort:skip
    debug_mode = os.getenv("DEBUG")  # noqa isort:skip
    config.load(devel_mode=devel_mode)  # noqa isort:skip

    """Creates and initializes Flask app"""

    app_opts = app_opts or {}
    app = Flask(__name__, **app_opts)

    # Rejects all unknown requests
    FlaskMiddleware(app)

    def set_cache_control(response):
        response.cache_control.no_cache = True
        return response

    # API responses mustn`t be cached
    app.after_request(set_cache_control)

    logging.setup(
        names=("yc", "werkzeug", "psh", "cloud"),
        devel_mode=devel_mode, debug_mode=debug_mode,
        context_fields=[
            ("request_id", "r"),
            ("request_uid", "u"),
            ("operation_id", "o"),
            ("user_id", "iam_uid"),
        ])
    log = logging.get_logger(__name__)
    log.info("Config devel mode = {}, Debug mode = {}".format(devel_mode, debug_mode))
    app.register_blueprint(public_api_blueprint, url_prefix="/marketplace/v1alpha1/console")
    app.register_blueprint(private_api_blueprint, url_prefix="/marketplace/v1alpha1/private")

    return app
