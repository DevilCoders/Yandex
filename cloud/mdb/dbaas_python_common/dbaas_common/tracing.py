# -*- coding: utf-8 -*-
"""
Tracing helpers
"""

from functools import wraps
import logging
import opentracing
from traceback import format_exc

from opentracing_instrumentation.interceptors import ClientInterceptors, OpenTracingInterceptor
from grpc_opentracing import open_tracing_client_interceptor, ActiveSpanSource, SpanDecorator
from grpc_opentracing.grpcext import intercept_channel


class RequestsInterceptor(OpenTracingInterceptor):
    """
    Requests interceptor that adds request_id and remote_address to traces
    """

    def __init__(self, sentry):
        self._sentry = sentry

    def process(self, request, span):
        try:
            req_id = request.request.headers['X-Request-Id']
            span.set_tag('request_id', req_id)
        except KeyError:
            if self._sentry:
                self._sentry.captureException()
        remote_address = request.request.headers.get('X-Forwarded-For')
        if remote_address:
            span.set_tag('remote_address', remote_address)


def init_requests_interceptor(sentry):
    ClientInterceptors.append(RequestsInterceptor(sentry))


def init_tracing(app, config):
    """
    Initialize opentracing client
    """

    try:
        if config['disabled']:
            return

        # Use function level import, cause sometimes we have a circular import
        # https://paste.yandex-team.ru/4488008
        from jaeger_client import Config

        cfg = {
            'reporter_queue_size': config['queue_size'],
            'sampler': config['sampler'],
            'logging': config['logging'],
        }
        if not config.get('use_env_vars'):
            cfg['local_agent'] = config['local_agent']

        ot_config = Config(
            config=cfg,
            service_name=config['service_name'],
            validate=True,
        )

        app.tracer = ot_config.initialize_tracer()
    except Exception:
        log = logging.getLogger('tracer')
        log.error("Unable to initialize tracer: %s", format_exc())


def trace(op_name, ignore_active_span=False):
    """
    Decorator for tracing operation.
    """

    def wrapper(callback):
        """
        Wrapper function (returns internal wrapper)
        """

        @wraps(callback)
        def trace_wrapper(*args, **kwargs):
            with opentracing.global_tracer().start_active_span(
                op_name, finish_on_close=True, ignore_active_span=ignore_active_span
            ):
                return callback(*args, **kwargs)

        return trace_wrapper

    return wrapper


def set_tag(tag, value):
    """
    Helper for setting tag on active span if any
    """
    span = opentracing.global_tracer().active_span
    if span is not None:
        span.set_tag(tag, value)


class GlobalSpanSource(ActiveSpanSource):
    def get_active_span(self):
        return opentracing.global_tracer().active_span


def grpc_channel_tracing_interceptor(channel):
    tracer_interceptor = open_tracing_client_interceptor(
        opentracing.global_tracer(), active_span_source=GlobalSpanSource(), span_decorator=MetadataSpanDecorator()
    )
    return intercept_channel(channel, tracer_interceptor)


class MetadataSpanDecorator(SpanDecorator):
    def __call__(self, span, rpc_info):
        if rpc_info.metadata is not None:
            metadict = dict(rpc_info.metadata)
            span.set_tag('request_id', metadict.get('x-request-id', 'none'))
