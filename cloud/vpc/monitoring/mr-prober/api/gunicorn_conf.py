from copy import deepcopy
import logging
import multiprocessing
from typing import Any, Dict

from gunicorn.glogging import CONFIG_DEFAULTS as gunicorn_default_logging_config
from prometheus_client import multiprocess

import settings


def logging_config_dict() -> Dict[str, Any]:
    config = deepcopy(gunicorn_default_logging_config)
    config.update(settings.LOGGING_CONFIG)
    # gunicorn uses a separate field in its config to override the "root" logger
    # and will give an error if you try to override "root" inside "loggers"
    if "" in config["loggers"]:
        config["root"] = config["loggers"].pop("")
    # disable_existing_loggers=True disables part of the gunicorn output at startup, so we return it to False
    config["disable_existing_loggers"] = False
    return config


def child_exit(server, worker):
    # Settings for Prometheus Multiprocess Mode
    # See https://github.com/prometheus/client_python#multiprocess-mode-eg-gunicorn
    multiprocess.mark_process_dead(worker.pid)


cores = multiprocessing.cpu_count()
workers_per_core = settings.API_WORKERS_PER_CORE

# Gunicorn config variables
loglevel = logging.getLevelName(settings.LOG_LEVEL).lower()
logconfig_dict = logging_config_dict()
workers = max(int(workers_per_core * cores), 2)
bind = f"{settings.HOST}:{settings.PORT}"
if settings.ADDITIONAL_PORT is not None:
    bind = [bind, f"{settings.HOST}:{settings.ADDITIONAL_PORT}"]
errorlog = "-"
accesslog = "-"
worker_tmp_dir = "/dev/shm"
graceful_timeout = 120
timeout = 120
keepalive = 5
