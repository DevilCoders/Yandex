# -*- coding: utf-8 -*-
"""
Tracing init helper
"""

from flask_opentracing import FlaskTracing

from dbaas_common.tracing import init_requests_interceptor, init_tracing as ot_init


def init_tracing(app):
    """
    Attach tracing client with config to app
    """

    ot_init(app, app.config['TRACING'])
    if app.tracer:
        app.flask_tracing = FlaskTracing(app.tracer, True, app)

    init_requests_interceptor(app.raven_client)
