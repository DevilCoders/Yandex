import os
from timeit import default_timer

import flask
from flask import request
from prometheus_client import Counter, Histogram, Gauge, CollectorRegistry, start_http_server, multiprocess

from .release import get_release
from .config import app_config


_APP_INFO = Gauge("app_info", "Application Info", ["release"], multiprocess_mode='min')
_REQUEST_TOTAL = Counter(
    "request_count",
    "A counter of requests",
    ["method", "path", "code"],
)
_REQUEST_DURATION = Histogram(
    "request_duration_seconds",
    "A histogram of latencies for requests",
    ["method", "path"],
)
_QUERY_TOTAL = Counter(
    "query_total",
    "A counter of queries",
    ["query_name", "success"],
)
_QUERY_DURATION = Histogram(
    "query_duration_seconds",
    "A histogram of latencies for queries",
    ["query_name"],
)


class QueryObserver:
    def __init__(self, name: str) -> None:
        self._name = name
        self._start_at = default_timer()

    def __enter__(self) -> None:
        pass

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        execution_time = default_timer() - self._start_at
        _QUERY_TOTAL.labels(self._name, 'true' if exc_type is None else 'false').inc()  # type: ignore
        _QUERY_DURATION.labels(self._name).observe(execution_time)  # type: ignore


def _before_request():
    """
    Remember start 'time' of a request
    """
    flask.g.request_start_at = default_timer()


def _after_request(response):
    """
    Observe metrics for each request
    """
    if request.url_rule is None:
        # Ignore requests without url_rule - they are invalid
        return
    request_time = default_timer() - flask.g.request_start_at
    # Use url_rule instead of path, cause path contains parameters
    handle = str(request.url_rule)
    _REQUEST_DURATION.labels(request.method, handle).observe(request_time)  # type: ignore
    _REQUEST_TOTAL.labels(request.method, handle, response.status_code).inc()  # type: ignore
    return response


def _should_start_metrics_server() -> bool:
    try:
        import uwsgi  # type: ignore

        return os.getpid() == uwsgi.masterpid()  # type: ignore
    except ImportError:
        return True


def init_metrics(app: flask.Flask) -> None:
    """
    Init /metrics
    """
    metrics_config = app_config().get('METRICS')
    if not metrics_config:
        app.logger.info('/metrics endpoint disabled in config')
        return

    app.before_request(_before_request)
    app.after_request(_after_request)
    _APP_INFO.labels(get_release()).set(1)  # type: ignore

    # Multiprocessing require additional setup
    # - Custom collectors in code
    # - multiproc_dir configuration
    # https://github.com/prometheus/client_python#multiprocess-mode-eg-gunicorn
    registry = CollectorRegistry()
    if 'prometheus_multiproc_dir' in os.environ and 'PROMETHEUS_MULTIPROC_DIR' not in os.environ:
        # multiproc dir can be configured with `PROMETHEUS_MULTIPROC_DIR`,
        # but in older prometheus-client it's called `prometheus_multiproc_dir`
        # and was deprecated in flavor to upper-case notation.
        app.logger.warning('specify prometheus_multiproc_dir and PROMETHEUS_MULTIPROC_DIR environment variables too')
    multiprocess.MultiProcessCollector(registry)

    if _should_start_metrics_server():
        port = metrics_config.get('port', 6060)
        start_http_server(port=port, addr=metrics_config.get('addr', 'localhost'), registry=registry)
        app.logger.info('started /metrics at %r port', port)
