# -*- coding: utf-8 -*-
"""
DBaaS Internal API SLB Check handler namespace
"""

import logging
import os
import time

import marshmallow
from flask import current_app, g
from flask.views import MethodView
from flask_restful import abort

from . import API, marshal
from ..utils import config
from ..utils.logs import get_logger
from ..utils.request_context import get_x_request_id
from .schemas.fields import Str


class SlbCheckSchema(marshmallow.Schema):
    """
    SLB Check response schema
    """

    status = Str()


def _log_req(message, req_id, start_at):
    exec_time = time.time() - start_at
    extra = {
        'request_id': req_id,
        'execution_time': 1000 * exec_time,
    }
    logger = logging.LoggerAdapter(get_logger(), extra)
    log_func = logger.info if exec_time > 1 else logger.debug
    log_func(message)


@API.resource('/ping')
class SlbCheckResource(MethodView):
    """
    SLB Check
    """

    @marshal.with_schema(SlbCheckSchema)
    def get(self):
        """
        Return OK on if DB is available and close file does not exist
        """
        if os.path.exists(config.get_close_path()):
            abort(405, message='Closed by close file')

        req_id = get_x_request_id()

        start_at = time.time()
        auth_provider = current_app.config['AUTH_PROVIDER'](current_app.config)
        auth_provider.ping()
        _log_req('http-ping, auth ping request', req_id, start_at)

        start_at = time.time()
        g.metadb.query('ping', fetch=False)
        _log_req('http-ping, metadb simple query', req_id, start_at)

        return {'status': 'OK'}
