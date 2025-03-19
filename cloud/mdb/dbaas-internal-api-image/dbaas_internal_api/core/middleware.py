# -*- coding: utf-8 -*-
"""
DBaaS Internal API Middlewares
"""

import json
import logging
import os
import time
import traceback
from functools import wraps

from flask import current_app, g, request
from flask_restful import abort

from ..utils.request_context import get_x_request_id
from .logs import mask_fields
from .stat import STAT

READ_ONLY_METHODS = ['GET', 'HEAD']


def stat_middleware(callback):
    """
    Stat Middleware
    """

    @wraps(callback)
    def stat_wrapper(*args, **kwargs):
        """
        Stat wrapper function (counts timing and status)
        """
        start = time.time()
        try:
            res = callback(*args, **kwargs)
            status_code = res.status_code
            return res
        except Exception as exc:
            status_code = getattr(exc, 'code', 503) or 503
            raise
        finally:
            total = (time.time() - start) * 1000

            stats = []
            endpoint = request.endpoint
            cluster_type = kwargs.get('cluster_type')
            cluster_metric = f"cluster_{cluster_type}_{endpoint}".replace('_cluster', '')
            if cluster_type is not None and cluster_metric in STAT:
                stats.append(STAT[cluster_metric])

            if endpoint in STAT:
                stats.append(STAT[endpoint])

            stat = STAT['common']
            stat['count'] += 1
            if status_code < 299:
                stat['count_2xx'] += 1
            elif status_code < 399:
                stat['count_3xx'] += 1
            elif status_code < 499 or status_code == 501:
                # 501 is Not Implemented, so count it in 400x group
                stat['count_4xx'] += 1
            else:
                stat['count_5xx'] += 1
                for endpoint_stat in stats:
                    endpoint_stat['count_5xx'] += 1
            for bucket in stat['timings']:
                if total < bucket:
                    stat['timings'][bucket] += 1
            for endpoint_stat in stats:
                endpoint_stat['count_total'] += 1

    return stat_wrapper


def _report_exception(code):
    # Handle only server errors
    if code != 503 or not getattr(current_app, 'raven_client', None):
        return
    try:
        current_app.raven_client.tags_context(
            {
                'endpoint': request.endpoint,
                'method': request.method,
            }
        )
        current_app.raven_client.user_context(
            {
                'user_agent': str(request.user_agent),
                'request_body': mask_fields(request),
                'user_id': getattr(g, 'user_id', None),
                'id': getattr(g, 'user_id', None),
                'folder_id': getattr(g, 'folder', {}).get('folder_ext_id'),
                'cloud_id': getattr(g, 'cloud', {}).get('cloud_ext_id'),
                'request_id': get_x_request_id(),
                'real_ip': request.environ.get('HTTP_X_REAL_IP', None),
            }
        )
        current_app.raven_client.captureException()
    except Exception as report_exc:
        logger = logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER'])
        logger.error('Unable to report error to sentry: %s', repr(report_exc))


def log_middleware(callback):
    """
    Middleware to log tracebacks of all errors
    """

    @wraps(callback)
    def log_wrapper(*args, **kwargs):
        """
        Log wrapper function
        """

        try:
            return callback(*args, **kwargs)
        except Exception as exc:
            logger = logging.getLogger(current_app.config['LOGCONFIG_BACKGROUND_LOGGER'])
            logger.error(traceback.format_exc(), extra={'request': request})
            code = getattr(exc, 'code', 503)
            if not isinstance(code, int):
                code = 503

            _report_exception(code)

            raise

    return log_wrapper


def json_body_middleware(callback):
    """
    Middleware to ensure that passed body is json-decodeable
    """

    @wraps(callback)
    def json_body_wrapper(*args, **kwargs):
        """
        Json-body decode function
        """
        if request.method not in READ_ONLY_METHODS and request.data:
            try:
                json.loads(request.data.decode('utf-8'))
            except json.JSONDecodeError:
                abort(400, message='Request body is not a valid JSON')

        return callback(*args, **kwargs)

    return json_body_wrapper


def read_only_middleware(callback):
    """
    Middleware to disable write requests if ro-flag is set
    """

    @wraps(callback)
    def read_only_wrapper(*args, **kwargs):
        """
        Read-only wrapper function
        """

        if request.method not in READ_ONLY_METHODS and os.path.exists(current_app.config['READ_ONLY_FLAG']):
            abort(503, message='Read-only')

        return callback(*args, **kwargs)

    return read_only_wrapper


def tracing_middleware(callback):
    """
    Middleware to add info to all incoming traces
    """

    @wraps(callback)
    def trace_wrapper(*args, **kwargs):
        """
        Trace wrapper function
        """

        span = current_app.flask_tracing.get_span(request)
        span.set_tag('request_id', get_x_request_id())

        return callback(*args, **kwargs)

    return trace_wrapper
