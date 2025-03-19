"""Flask application classes"""

import argparse
import datetime
import logging
import mimetypes
import uuid

from library.python import resource

import flask
from flask_log_request_id import RequestID, current_request_id
import jinja2
import gunicorn.app.base
import schematics.exceptions
from schematics.models import Model
from schematics.types import IntType, BooleanType, ModelType

from bootstrap.common.config import BootstrapConfigMixin
from bootstrap.common.rdbms.db import DbConfig, Db

from .core.auth import AuthorizerConfig, Authorizer
from .logging import LogConfig, init_logging, get_logger
from .routes import api as flask_restplus_api


class ProfilerConfig(Model):
    enabled = BooleanType(default=False)
    limit = IntType(default=30)


class AppConfig(Model, BootstrapConfigMixin):
    _CONFIG_FILE = "/etc/yc/bootstrap/api/config.yaml"
    _SECRET_CONFIG_FILE = "/etc/yc/bootstrap/api/secret_config.yaml"

    port = IntType(default=23712)
    production = BooleanType(default=False)
    workers = IntType(default=1)
    logging = ModelType(LogConfig, default=LogConfig)
    db = ModelType(DbConfig, default=DbConfig)
    auth = ModelType(AuthorizerConfig, default=AuthorizerConfig)
    timeout = IntType(default=30)
    debug = BooleanType(default=False)
    profiler = ModelType(ProfilerConfig, default=ProfilerConfig)


class FromResourcesLoader(jinja2.BaseLoader):
    """Some templates are built in binary"""

    def get_source(self, environment, template: str) -> str:
        # check for template in resources
        template_content = resource.find("/templates/{}".format(template))
        if template_content:
            return template_content.decode("utf-8"), None, lambda: False

        raise jinja2.exceptions.TemplateNotFound(template)


def create_flask_app() -> flask.Flask:
    # create flask app
    flask_app = flask.Flask('bootstrap.api', static_folder=None)

    # allow templates to be loaded from resources
    my_loader = jinja2.ChoiceLoader([
        flask_app.jinja_loader,
        FromResourcesLoader(),
    ])
    flask_app.jinja_loader = my_loader

    # load static from resources
    @flask_app.route("/swaggerui/<path:filename>")
    def static_from_resources(filename) -> flask.Response:
        file_content = resource.find("/static/{}".format(filename))
        if file_content:
            return flask.Response(file_content, content_type=mimetypes.guess_type(filename)[0])
        flask.abort(404)

    flask_restplus_api.init_app(flask_app)

    flask_app.config['RESTPLUS_MASK_SWAGGER'] = False

    @flask_app.before_request
    def before_request():
        """Some preparation for better logging (CLOUD-30927)"""
        flask.request.start_time = datetime.datetime.now()

    RequestID(flask_app, request_id_generator=lambda: str(uuid.uuid4())[:8])

    @flask_app.after_request
    def append_request_id(response):
        response.headers.add('X-Request-Id', current_request_id())
        return response

    return flask_app


class TFlaskApplication(object):
    """Flask application class (used for debug purporses)"""

    def __init__(self, flask_app: flask.Flask, app_config: AppConfig):
        self.flask_app = flask_app
        self.port = app_config.port

        log = logging.getLogger('werkzeug')
        log.setLevel(logging.ERROR)

    def run(self) -> None:
        self.flask_app.run("::", port=self.port)

    def stop(self) -> None:
        self.flask_app.stop()


class TGunicornApplication(gunicorn.app.base.BaseApplication):
    """Gunicorn application class (to be used for used for production)"""

    def __init__(self, flask_app: flask.Flask, app_config: AppConfig):
        self.flask_app = flask_app
        self.port = app_config.port
        self.workers = app_config.workers
        self.timeout = app_config.timeout

        super(TGunicornApplication, self).__init__()

    def load_config(self) -> None:
        self.cfg.set("workers", self.workers)
        self.cfg.set("bind", "[::]:{0}".format(self.port))
        self.cfg.set("timeout", self.timeout)

    def load(self) -> flask.Flask:
        return self.flask_app


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Bootstrap api")

    AppConfig.update_parser(parser)

    return parser


def _app_config_from_options(options: argparse.Namespace) -> AppConfig:
    """Load options from config and override by command-line parameters"""
    try:
        app_config = AppConfig.from_argparse(options)
    except schematics.exceptions.DataError as e:
        fmtmsg = "Failed to validate config <{}> with schematics. Schematics Error:\n{}"
        raise Exception(fmtmsg.format(options.config, str(e))) from None

    app_config.validate()

    return app_config


def main(options) -> int:
    app_config = _app_config_from_options(options)

    flask_app = create_flask_app()

    # initialize all stuff
    flask_app.db = Db(app_config.db, logger=get_logger())
    flask_app.authner = Authorizer(app_config.auth)
    init_logging(app_config.logging)
    flask_app.debug = app_config.debug
    if app_config.profiler.enabled:
        from werkzeug.middleware.profiler import ProfilerMiddleware
        flask_app.config['PROFILE'] = True
        flask_app.wsgi_app = ProfilerMiddleware(flask_app.wsgi_app, restrictions=[app_config.profiler.limit])

    if app_config.production:
        app = TGunicornApplication(flask_app, app_config)
    else:
        app = TFlaskApplication(flask_app, app_config)

    app.run()

    return 0
