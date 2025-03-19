"""MDB IDM API Service."""
from flask import Flask
from flask_appconfig import AppConfig
from flask_restful import Api
import logging
import logging.config
from .views import init_routes
from .metadb import DB
from .tvm_constants import TVM_CONSTANTS
from .vault import VAULT


def init_logging(app):
    """Initialize logging system"""
    logging.getLogger('werkzeug').disabled = True
    logging.config.dictConfig(app.config['LOGCONFIG'])


def create_app():
    """Initialize MDB IDM API Service."""
    app = Flask(__name__)
    AppConfig(app)
    init_logging(app)
    DB.init_metadb(app.config)
    TVM_CONSTANTS.init_tvm(app.config)
    VAULT.init_vault(app.config)
    api = Api(app)
    init_routes(api)
    return app
