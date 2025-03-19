from prometheus_client import generate_latest
from prometheus_client import multiprocess
from prometheus_client.core import CollectorRegistry

from yc_common import logging

log = logging.get_logger(__name__)


def get_registry():
    registry = CollectorRegistry()

    multiprocess.MultiProcessCollector(registry)
    return registry


def setup_registry():
    try:
        from prometheus_client import core
        import uwsgi
        # Use uwsgi's worker_id rather than system pids
        core._ValueClass = core._MultiProcessValue(_pidFunc=uwsgi.worker_id)
    except Exception:
        # No uwsgi support
        log.info("no uwsgi, fallback to pid")

    get_registry()


def collect():
    return generate_latest(get_registry())
