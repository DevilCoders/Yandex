# coding=utf-8
import os
import sys
import traceback
import warnings
from logging.config import fileConfig
from uuid import uuid4
from cStringIO import StringIO
from copy import deepcopy

import psycopg2
from flask import Flask, jsonify, g, request
from voluptuous import MultipleInvalid
from werkzeug.exceptions import default_exceptions, HTTPException
from library.python import resource

from antiadblock.configs_api.lib.audit.audit_api import audit_api
from antiadblock.configs_api.lib.api_version import CURRENT_API_PREFIX, CURRENT_INTERNAL_API_PREFIX, PREVIOUS_INTERNAL_API_PREFIX
from antiadblock.configs_api.lib.auth.auth_api import auth_api
from antiadblock.configs_api.lib.auth.idm_api import idm_api
from antiadblock.configs_api.lib.context import init_context
from antiadblock.configs_api.lib.internal_api import internal_api
from antiadblock.configs_api.lib.argus.argus_api import argus_api
from antiadblock.configs_api.lib.argus.profiles import argus_profiles
from antiadblock.configs_api.lib.metrics.metrics_api import metrics_api
from antiadblock.configs_api.lib.dashboard.dashboard_api import dashboard_api
from antiadblock.configs_api.lib.dashboard.config import DASHBOARD_API_CONFIG
from antiadblock.configs_api.lib.heatmap.heatmap import heatmap_api
from antiadblock.configs_api.lib.bot.bot import bot_api
from antiadblock.configs_api.lib.db import init_database, db
from antiadblock.configs_api.lib.service_api.api import api

# init logging before create flask app http://flask.pocoo.org/docs/dev/logging/
logging_config = StringIO(resource.find("logging.ini"))
fileConfig(logging_config)

app = Flask(__name__)

# TODO: remove RAW API when front move to v1 api version
app.register_blueprint(api)
app.register_blueprint(internal_api)
app.register_blueprint(argus_api)
app.register_blueprint(argus_profiles)
app.register_blueprint(audit_api)
app.register_blueprint(idm_api)
app.register_blueprint(heatmap_api)
app.register_blueprint(bot_api)
app.register_blueprint(dashboard_api)
app.register_blueprint(api, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(argus_api, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(argus_profiles, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(internal_api, url_prefix=PREVIOUS_INTERNAL_API_PREFIX)
app.register_blueprint(internal_api, url_prefix=CURRENT_INTERNAL_API_PREFIX)
app.register_blueprint(audit_api, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(auth_api, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(metrics_api, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(idm_api, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(heatmap_api, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(dashboard_api, url_prefix=CURRENT_API_PREFIX)
app.register_blueprint(bot_api, url_prefix=CURRENT_API_PREFIX)

app.config['HOST_DOMAIN'] = os.environ.get('HOST_DOMAIN', 'develop.antiblock.yandex.ru')

app.config['SQLALCHEMY_DATABASE_URI'] = os.environ.get('DATABASE_URL',
                                                       'postgresql+psycopg2://antiadb:postgres@localhost/configs')
app.config['WEBMASTER_API_URL'] = os.environ.get('WEBMASTER_API_URL',
                                                 'https://webmaster3-internal.test.in.yandex.net')
app.config['INFRA_API_URL'] = os.environ.get('INFRA_API_URL', 'https://infra-api-test.yandex-team.ru/v1/')

DEFAULT_PRIVATE_KEY = resource.find("keys/default.key")

app.config['PRIVATE_CRYPT_KEY'] = os.environ.get("PRIVATE_CRYPT_KEY", DEFAULT_PRIVATE_KEY)

if app.config['PRIVATE_CRYPT_KEY'] == DEFAULT_PRIVATE_KEY:
    PUBLIC_KEY = resource.find("keys/default.pub")
else:
    PUBLIC_KEY = resource.find("keys/key.pub")

app.config['PUBLIC_CRYPT_KEY'] = PUBLIC_KEY
app.config['ENVIRONMENT_TYPE'] = os.environ.get("ENVIRONMENT_TYPE", "DEVELOPMENT").upper()
app.config['TVM_SECRET'] = os.environ.get("TVM_SECRET", "CF5_BJq74y4ns3anZ_XXqg")
app.config['CHARTS_TOKEN'] = os.environ.get("CHARTS_TOKEN")
app.config['TOOLS_TOKEN'] = os.environ.get("TOOLS_TOKEN")
app.config['SANDBOX_TOKEN'] = os.environ.get("SANDBOX_TOKEN")
app.config['YT_TOKEN'] = os.environ.get("YT_TOKEN")
app.config['INFRA_TOKEN'] = os.environ.get("INFRA_TOKEN")
app.config['IS_PROD'] = app.config["ENVIRONMENT_TYPE"] == "PRODUCTION"


WEBMASTER_TEST_TVM_CLIENT_ID = 2000286
WEBMASTER_PROD_TVM_CLIENT_ID = 2000036

IDM_TEST_TVM_CLIENT_ID = 2001602
IDM_PROD_TVM_CLIENT_ID = 2001600

# namespace "Antiadblock Partners"
INFRA_TEST_NAMESPACE_ID = 656
INFRA_PROD_NAMESPACE_ID = 629

# id TVM Ресурсов из ABC
# https://abc.yandex-team.ru/services/antiadblock/resources/?supplier=14&type=47&state=requested&state=approved&state=granted&view=consuming
AAB_ADMIN_TEST_TVM_CLIENT_ID = 2000627  # Adminka stage and dev stands
AAB_ADMIN_PRODUCTION_TVM_CLIENT_ID = 2000629  # Adminka production
AAB_CRYPROX_PRODUCTION_TVM_CLIENT_ID = 2001021  # Cryprox production
AAB_CRYPROX_DEV_TVM_CLIENT_ID = 2001023  # Cryprox local development
AAB_CHECKLIST_JOB_PRODUCTION_TVM_CLIENT_ID = 2001149  # Check list job production
AAB_CHECKLIST_JOB_DEV_TVM_CLIENT_ID = 2001137  # Check list job local development and testing
AAB_SANDBOX_MONITORING_TVM_ID = 2002631  # Sandbox jobs
ERROR_BUSTER_TVM_CLIENT_ID = 2010338  # ErrorBuster
MONRELAY_TVM_CLIENT_ID = 2010820  # MONRELAY scheduler
ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID = 2019101  # telegram bot


if app.config["ENVIRONMENT_TYPE"] == "PRODUCTION":
    app.config["WEBMASTER_TVM_CLIENT_ID"] = WEBMASTER_PROD_TVM_CLIENT_ID
    app.config["INFRA_NAMESPACE_ID"] = INFRA_PROD_NAMESPACE_ID
    app.config["AAB_ADMIN_TVM_CLIENT_ID"] = AAB_ADMIN_PRODUCTION_TVM_CLIENT_ID
    app.config["AAB_CRYPROX_TVM_CLIENT_ID"] = AAB_CRYPROX_PRODUCTION_TVM_CLIENT_ID
    app.config["AAB_CHECKLIST_JOB_TVM_CLIENT_ID"] = AAB_CHECKLIST_JOB_PRODUCTION_TVM_CLIENT_ID
    app.config["IDM_TVM_CLIENT_ID"] = IDM_PROD_TVM_CLIENT_ID
    app.config["AAB_SANDBOX_MONITORING_TVM_ID"] = AAB_SANDBOX_MONITORING_TVM_ID
    app.config["ERROR_BUSTER_TVM_CLIENT_ID"] = ERROR_BUSTER_TVM_CLIENT_ID
    app.config["MONRELAY_TVM_CLIENT_ID"] = MONRELAY_TVM_CLIENT_ID
    app.config["ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID"] = ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID
elif app.config["ENVIRONMENT_TYPE"] == "TESTING":
    app.config["WEBMASTER_TVM_CLIENT_ID"] = WEBMASTER_TEST_TVM_CLIENT_ID
    app.config["INFRA_NAMESPACE_ID"] = INFRA_TEST_NAMESPACE_ID
    app.config["AAB_ADMIN_TVM_CLIENT_ID"] = AAB_ADMIN_TEST_TVM_CLIENT_ID
    app.config["AAB_CRYPROX_TVM_CLIENT_ID"] = AAB_CRYPROX_DEV_TVM_CLIENT_ID
    app.config["AAB_CHECKLIST_JOB_TVM_CLIENT_ID"] = AAB_CHECKLIST_JOB_DEV_TVM_CLIENT_ID
    app.config["IDM_TVM_CLIENT_ID"] = IDM_TEST_TVM_CLIENT_ID
    app.config["AAB_SANDBOX_MONITORING_TVM_ID"] = AAB_SANDBOX_MONITORING_TVM_ID
    app.config["ERROR_BUSTER_TVM_CLIENT_ID"] = ERROR_BUSTER_TVM_CLIENT_ID
    app.config["MONRELAY_TVM_CLIENT_ID"] = MONRELAY_TVM_CLIENT_ID
    app.config["ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID"] = ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID
else:
    app.config["WEBMASTER_TVM_CLIENT_ID"] = WEBMASTER_TEST_TVM_CLIENT_ID
    app.config["INFRA_NAMESPACE_ID"] = INFRA_TEST_NAMESPACE_ID
    app.config["AAB_ADMIN_TVM_CLIENT_ID"] = AAB_ADMIN_TEST_TVM_CLIENT_ID
    app.config["AAB_CRYPROX_TVM_CLIENT_ID"] = AAB_CRYPROX_DEV_TVM_CLIENT_ID
    app.config["AAB_CHECKLIST_JOB_TVM_CLIENT_ID"] = AAB_CHECKLIST_JOB_DEV_TVM_CLIENT_ID
    app.config["IDM_TVM_CLIENT_ID"] = IDM_TEST_TVM_CLIENT_ID
    app.config["AAB_SANDBOX_MONITORING_TVM_ID"] = AAB_SANDBOX_MONITORING_TVM_ID
    app.config["ERROR_BUSTER_TVM_CLIENT_ID"] = ERROR_BUSTER_TVM_CLIENT_ID
    app.config["MONRELAY_TVM_CLIENT_ID"] = MONRELAY_TVM_CLIENT_ID
    app.config["ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID"] = ANTIADB_SUPPORT_BOT_TVM_CLIENT_ID


# In case we ever want to turn it on, but it's unlikely, cause flask_sqlalchemy event system is depricated
#   (see pr added the warning: https://github.com/mitsuhiko/flask-sqlalchemy/pull/256)
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = bool(os.environ.get("SQLALCHEMY_TRACK_MODIFICATIONS"))


def init_dashboard_api_config():
    dashboard_api_config = deepcopy(DASHBOARD_API_CONFIG)
    for group in dashboard_api_config.get("groups"):
        for check in group["checks"]:
            check.update({'ttl': check.get('ttl', group.get('group_ttl')),
                          'update_period': check.get('update_period', group.get('group_update_period'))})
        del group['group_ttl'], group['group_update_period']
    return dashboard_api_config


app.config['DASHBOARD_API_CONFIG'] = init_dashboard_api_config()

# do not spam with Unverified HTTPS warnings
if not sys.warnoptions:
    warnings.simplefilter("module")


def make_json_error(ex, **extra):
    """
    All error responses that you don't specifically manage yourself will have application/json content type,
    and will contain JSON like this:
    {
      "message": "Method Not Allowed",
      "status_code": 405,
      "request_id": "3f1987de-1b23-4344-bc95-ade5665c4508"
    }
    """
    exc_type, exc_value, exc_traceback = sys.exc_info()

    status_code = isinstance(ex, HTTPException) and ex.code or 500
    message = "{0.__class__.__name__}".format(ex)
    if 499 < status_code < 599:
        app.logger.error("{0.__class__.__name__} : {0}".format(ex), extra=dict(
            exception="".join(traceback.format_exception(exc_type, exc_value, exc_traceback, limit=3)),
            **extra))
    else:
        # do not log wrong url attempts
        if status_code != 404 and "404 Not Found: The requested URL was not found on the server" not in ex.description:
            app.logger.warning("{0.__class__.__name__} : {0}".format(ex), extra=dict(
                exception="".join(traceback.format_exception(exc_type, exc_value, exc_traceback, limit=3)),
                **extra))

    return jsonify(message=message, status_code=status_code, request_id=g.get("request_id", None), **extra), status_code


for code in default_exceptions.iterkeys():
    app.register_error_handler(code, make_json_error)

for exception in [psycopg2.DatabaseError, psycopg2.DataError]:
    app.register_error_handler(exception, make_json_error)


@app.errorhandler(MultipleInvalid)
def handle_validation_error(error):
    """
    Error handler for validation errors.
    Wraps voluptuous' MultipleInvalid to a frontend-understanded json like ::

      {
          "message": "Validation error",
          "properties": [
              {
                path: ["path", 0, "to", "bad", "element", 0],
                message: "Human-readable error message about that element"
              }
          ]
      }

    :param error:
    :return:
    """
    def primitivize(val):
        """
        Transforms objects to unicode str, but does not affect integers.
        Used in path serialization.
        :param val:
        :return:
        """
        if isinstance(val, int):
            return val
        return unicode(val)
    properties = [{"path": map(primitivize, e.path), "message": unicode(e.error_message)} for e in error.errors]

    app.logger.error('Validation error', extra=dict(error_properties=properties))

    return jsonify(dict(message=u'Ошибка валидации', properties=properties)), 400


def check_configuration():
    """
      Check that application has full configuration.

      If this function raises an exception request is not served.
      Each new request will retry this function if it failed earlier.
      This will cause `GET /ping` to fail and prevent traffic to be served by an ill-configured application.
      """

    for param in ['PRIVATE_CRYPT_KEY', 'SQLALCHEMY_DATABASE_URI']:
        if not app.config.get(param):
            raise ValueError('Configuration parameter %s is not given' % param)
    if app.config['ENVIRONMENT_TYPE'] == 'PRODUCTION':
        if app.config['HOST_DOMAIN'] == 'develop.antiblock.yandex.ru':
            raise ValueError("HOST_DOMAIN couldn't be default in Production environment")
        if app.config['PRIVATE_CRYPT_KEY'] == DEFAULT_PRIVATE_KEY:
            raise ValueError("PRIVATE_CRYPT_KEY couldn't be default in Production environment")
        if app.config['TVM_SECRET'] == "BjQR2kXaTXfE7dOlNL7kAg":
            raise ValueError("TVM_SECRET couldn't be default in Production environment")
        if app.config["WEBMASTER_API_URL"] == "https://webmaster3-internal.test.in.yandex.net":
            raise ValueError("WEBMASTER_API_URL couldn't be default in Production environment")
        if app.config["INFRA_API_URL"] == "https://infra-api-test.yandex-team.ru/v1/":
            raise ValueError("INFRA_API_URL couldn't be default in Production environment")
        if app.config["INFRA_NAMESPACE_ID"] == INFRA_TEST_NAMESPACE_ID:
            raise ValueError("INFRA_SERVICE_ID couldn't be default in Production environment")
        if app.config["AAB_CRYPROX_TVM_CLIENT_ID"] == AAB_CRYPROX_DEV_TVM_CLIENT_ID:
            raise ValueError("AAB_CRYPROX_TVM_CLIENT_ID couldn't be default in Production environment")
        if app.config["AAB_CHECKLIST_JOB_TVM_CLIENT_ID"] == AAB_CHECKLIST_JOB_DEV_TVM_CLIENT_ID:
            raise ValueError("AAB_CHECKLIST_JOB_TVM_CLIENT_ID couldn't be default in Production environment")
        if app.config["IDM_TVM_CLIENT_ID"] == IDM_TEST_TVM_CLIENT_ID:
            raise ValueError("IDM_TVM_CLIENT_ID couldn't be default in Production environment")

        app.logger.warning("Application started with default PRIVATE_CRYPT_KEY")


@app.after_request
def apply_cors(response):
    origin = request.headers.get("Origin")
    if origin and origin.strip().startswith('https') and origin.strip().endswith('.yandex.ru'):
        response.headers["Access-Control-Allow-Origin"] = origin or "*"
        response.headers["Access-Control-Allow-Headers"] = "Content-Type, X-Requested-With"
        response.headers["Access-Control-Allow-Credentials"] = "true"
        response.headers["Access-Control-Allow-Methods"] = "POST, GET, PUT, DELETE, PATCH"
    return response


@app.before_request
def set_request_id():
    """
    Saves request id for logging and user notification purposes
    :return:
    """
    g.request_id = str(uuid4())
    g.logging_context = dict(request_id=g.request_id, url=request.full_path)


# noinspection PyUnusedLocal
@app.teardown_request
def teardown_request(exception):
    db.session.close()


@app.route('/ping')
def ping():
    if not db.session.execute('SELECT 1'):
        return "DB request failed", 500
    return 'OK'


def init_app():
    with app.app_context():
        check_configuration()
        db.init_app(app)
        init_database(app, db)
        init_context(app)


def main():
    init_app()
    app.run(host="::", port=80, threaded=True)


# It is suppose to run only as wsgi app in production environment. But you can use this way in dev at your discretion
if __name__ == '__main__':
    main()
