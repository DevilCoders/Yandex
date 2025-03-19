# -*- coding: utf-8 -*-
"""
App init
"""
import requests  # noqa
from opentracing_instrumentation.client_hooks.requests import patcher  # type: ignore

from flask import Flask
from flask_appconfig import AppConfig
import prometheus_flask_exporter

from . import modules  # noqa
from .apis import init_apis
from .converters import ClusterTypeConverter
from .core import init_logging, init_raven, init_stat, init_tracing
from .dbs.postgresql import DB as META_DB
from .health import MDBH as MDB_HEALTH


patcher.install_patches()


def create_app():
    """
    Initialize DBaaS API application
    """
    metrics = prometheus_flask_exporter.PrometheusMetrics.for_app_factory(
        defaults_prefix=prometheus_flask_exporter.NO_PREFIX, group_by='endpoint', default_labels={}
    )
    app = Flask(__name__)
    AppConfig(app, None)

    init_logging(app)
    init_raven(app)
    init_tracing(app)
    app.url_map.converters['ctype'] = ClusterTypeConverter
    init_apis(app)

    metadb_creds = None
    if app.config.get('METADB_USER') and app.config.get('METADB_PASSWORD'):
        metadb_creds = {"user": app.config.get('METADB_USER'), "password": app.config.get('METADB_PASSWORD')}

    META_DB.init_metadb(
        app.config['METADB'],
        app.config['LOGCONFIG_BACKGROUND_LOGGER'],
        metadb_creds,
    )
    MDB_HEALTH.init_mdbhealth(
        app.config['MDBH_PROVIDER'](app.config['MDBHEALTH']),
        app.config['LOGCONFIG_BACKGROUND_LOGGER'],
    )
    init_stat(app)
    metrics.init_app(app)
    return app
