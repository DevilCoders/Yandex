"""Application logging"""

import datetime
import http
import json
import logging
import logging.handlers
from typing import Dict, Optional, Tuple

from flask import request
from flask_log_request_id import RequestIDLogFilter
from schematics.models import Model
from schematics.types import IntType, BooleanType, StringType, ModelType

try:
    from cysystemd.journal import JournaldLogHandler
except ModuleNotFoundError:
    JournaldLogHandler = None

from bootstrap.common.exceptions import BootstrapError


_LOG_DATA_MAX_LEN = 1000  # first bytest of request/response data to save to logs

_LOG_LEVELS = [
    logging.CRITICAL,
    logging.ERROR,
    logging.WARNING,
    logging.INFO,
    logging.DEBUG
]

_LOG_FMT = "[%(asctime)s.%(msecs)03d]\t[REQ]\t[%(request_id)s]\t%(message)s"
_LOG_DATEFMT = "%d/%b/%y %H:%M:%S"


class JournaldLogConfig(Model):
    enabled = BooleanType(default=False)
    log_level = IntType(default=logging.INFO, choices=_LOG_LEVELS)


class FileLogConfig(Model):
    enabled = BooleanType(default=False)
    name = StringType(default=None)
    log_level = IntType(default=logging.INFO, choices=_LOG_LEVELS)


class LogConfig(Model):
    debug = BooleanType(default=False)
    file = ModelType(FileLogConfig, default=FileLogConfig)
    journald = ModelType(JournaldLogConfig, default=JournaldLogConfig)


def get_logger() -> logging.Logger:
    return logging.getLogger("bootstrap.api.log")


def init_logging(config: LogConfig) -> None:
    logger = get_logger()
    logger.setLevel(logging.DEBUG)
    logger.handlers = []

    if config.debug:
        handler = logging.StreamHandler()
        handler.setLevel(logging.DEBUG)
        logger.addHandler(handler)

    if config.file.enabled:
        handler = logging.handlers.WatchedFileHandler(config.file.name)
        handler.setLevel(config.file.log_level)
        handler.setFormatter(
            logging.Formatter(_LOG_FMT,
                              datefmt=_LOG_DATEFMT))
        handler.addFilter(RequestIDLogFilter())  # << Add request id contextual filter
        logger.addHandler(handler)

    if config.journald.enabled:
        if JournaldLogHandler is None:
            raise BootstrapError("Journald logger is not supported but requested in config")
        handler = JournaldLogHandler()
        handler.setLevel(config.journald.log_level)
        handler.setFormatter(
            logging.Formatter(_LOG_FMT,
                              datefmt=_LOG_DATEFMT))
        handler.addFilter(RequestIDLogFilter())  # << Add request id contextual filter
        logger.addHandler(handler)


def _rcut_data(s: str, max_len: int) -> Tuple[str, str]:
    assert max_len > 3

    if len(s) <= max_len:
        return s, ""

    return "{}...".format(s[:max_len-3]), " ({} of {} bytes)".format(max_len-3, len(s))


def message_to_access_log(response: Dict, response_code: http.HTTPStatus, tb: Optional[str] = None):
    """Write message to access log"""
    logger = get_logger()

    prefix_msg = '\t[{elapsed}]\t[{source_addr}]\t[{user}]\t"{url}"'.format(
        elapsed=(datetime.datetime.now() - request.start_time).total_seconds(),
        source_addr=request.remote_addr,
        user=getattr(request, "authentificated_user", None),
        url="{} {}".format(request.method, request.url),
    )

    if response_code >= http.HTTPStatus.INTERNAL_SERVER_ERROR:
        logger_func = logger.error
    else:
        logger_func = logger.info

    logger_func("{}\t{}".format(prefix_msg, response_code))

    # log request (CLOUD-30927)
    if request.data:
        request_data, request_cut_msg = _rcut_data(request.data, _LOG_DATA_MAX_LEN)
        logger_func("{}\tREQUEST_DATA{}\n{}".format(prefix_msg, request_cut_msg, request_data))

    # log response (CLOUD-30927)
    try:
        full_response_data = json.dumps(response, sort_keys=True, indent=4)
    except TypeError:
        full_response_data = "Failed to present response as json dump!!!"
    response_data, response_cut_msg = _rcut_data(full_response_data, _LOG_DATA_MAX_LEN)
    logger_func("{}\tRESPONSE_DATA{}\n{}".format(prefix_msg, response_cut_msg, response_data))

    # log traceback
    if tb:
        logger_func("{}\t{}".format(prefix_msg, tb))
