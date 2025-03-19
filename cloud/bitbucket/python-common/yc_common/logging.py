"""Logging support."""

###############################################################################
# Logging setup. Do not add any imports here.
###############################################################################

import logging
# Export these constants
from logging import DEBUG, INFO, WARNING, ERROR, CRITICAL

DEFAULT_LOG_MODULES = ("yc", "werkzeug", "psh")

class _Logger(logging.Logger):
    def makeRecord(self, name, level, fn, lno, msg, args, exc_info, func=None, extra=None, sinfo=None):
        """
        logging.Logger.makeRecord() doesn't allow to override record's members using extra parameter, so override this
        method to be able to alter caller info.
        """

        if extra and "real_caller" in extra:
            fn, lno, func = extra["real_caller"]

        return super().makeRecord(name, level, fn, lno, msg, args, exc_info, func=func, extra=extra, sinfo=sinfo)


# Attention: We have to unconditionally modify logger here because we need to modify it as soon as possible - before any
# logging.get_logger() call.
logging.Logger.manager.setLoggerClass(_Logger)

###############################################################################
# Logging setup finished. Actual module code after.
###############################################################################


import inspect
import sys
import time
import warnings

from yc_common.context import get_context
from yc_common.misc import ellipsis_string

_DEPRECATED_USAGE_WARNING = False

_MASKED_FIELDS = ("oauth_token", "oauthToken", "token", "iam_token", "iamToken", "jwt", "yandexPassportOauthToken")
_MASKING_VALUE = "[masked value]"
_MAX_HANDLER_CALL_DURATION = 1


class _LogRecord(logging.LogRecord):
    DEBUG_NAME_LENGTH = 25

    def __init__(self, *args, debug_mode=False, context_fields=(), ignore_context_for=(),  **kwargs):
        # journal logger sends all __dict__ fields as log record fields, so just store the context data in the log
        # record.
        self.__dict__.update(get_context().to_dict())

        super().__init__(*args, **kwargs)

        if debug_mode:
            self.debug_name = ellipsis_string(self.name + ":" + self.filename, self.DEBUG_NAME_LENGTH, ellipsis="*")
        else:
            self.debug_name = self.name

        self.__ignore_context_for = ignore_context_for
        self.__context_fields = context_fields

    def getMessage(self):
        message = super().getMessage()

        if self.name in self.__ignore_context_for:
            return message

        for field, name in self.__context_fields:
            value = getattr(self, field, None)
            if value is not None:
                message = "[{name}={value}] {message}".format(name=name, value=value, message=message)

        # We don't add "%(levelname)s: " to log formatter string and add message level directly to the message because
        # journalctl doesn't display log level and JournaldLogHandler doesn't use any formatters and logs only the
        # message string.
        return "{level}: {message}".format(level=self.levelname, message=message)


def get_logger(module_name=None, *, module=None, name=None):
    if name is not None:
        return logging.getLogger(name)

    if module_name is None:
        if module is None:
            parent_frame = inspect.currentframe().f_back
            module = inspect.getmodule(parent_frame)

            global _DEPRECATED_USAGE_WARNING
            if not _DEPRECATED_USAGE_WARNING:
                _DEPRECATED_USAGE_WARNING = True

                # This was the main approach of obtaining the logger but then it turned out that it's too expensive and
                # slows down service start up 2x times, so this approach has been deprecated. `get_logger(__name__)`
                # should be used instead.
                message = (
                    "{} module (and may be some other modules) use {}.get_logger() in a deprecated way "
                    "which may significantly slow down application startup.".format(module.__name__, __name__))
                warnings.warn(message)

        module_name = module.__name__

    if module_name.startswith("yc_"):
        name = "yc." + module_name[len("yc_"):]
    else:
        name = module_name

    return logging.getLogger(name)


log = get_logger(__name__)
CONFIGURED = False


def setup(names=DEFAULT_LOG_MODULES, ignore_context_for=("billing_metrics",), devel_mode=False,
          debug_mode=None, level=None, timestamp=None,
          process_name=None, context_fields=()):
    global CONFIGURED
    if CONFIGURED:
        raise Exception("The logger is already configured.")
    CONFIGURED = True

    if names is not None:
        names = [names] if isinstance(names, str) else list(names)

    context_fields = tuple(reversed(context_fields))

    to_stderr = True
    fallback_to_stderr = False

    if not devel_mode:
        try:
            from cysystemd.journal import JournaldLogHandler
        except ImportError:
            fallback_to_stderr = True
        else:
            to_stderr = False

    if debug_mode is None:
        debug_mode = devel_mode

    if timestamp is None:
        timestamp = to_stderr and not fallback_to_stderr

    if level is None:
        level = logging.DEBUG if debug_mode else logging.INFO

    logging.addLevelName(logging.DEBUG,    "D")
    logging.addLevelName(logging.INFO,     "I")
    logging.addLevelName(logging.WARNING,  "W")
    logging.addLevelName(logging.ERROR,    "E")
    logging.addLevelName(logging.CRITICAL, "C")

    logging.setLogRecordFactory(lambda *args, **kwargs: _LogRecord(*args, debug_mode=debug_mode, context_fields=context_fields,
                                                                   ignore_context_for=ignore_context_for, **kwargs))
    format = ""
    if timestamp:
        format += "%(asctime)s.%(msecs)03d "

    details = []

    if to_stderr and not fallback_to_stderr:
        if debug_mode:
            details.append("%(debug_name){debug_name_length}.{debug_name_length}s:%(lineno)04d".format(
                debug_name_length=_LogRecord.DEBUG_NAME_LENGTH))

    if process_name is not None:
        details.append(process_name)

    if details:
        format += " ".join("[" + detail + "]" for detail in details) + ": "

    # We don't add "%(levelname)s: " to log formatter string and add message level directly to the message because
    # journalctl doesn't display log level and JournaldLogHandler doesn't use any formatters and logs only the message
    # string.

    format += "%(message)s"
    date_format = "%Y.%m.%d %H:%M:%S"

    if devel_mode:
        from colorlog import ColoredFormatter
        formatter = ColoredFormatter("%(log_color)s" + format, date_format, log_colors={
            logging.getLevelName(logging.DEBUG):    "white",
            logging.getLevelName(logging.INFO):     "green",
            logging.getLevelName(logging.WARNING):  "yellow",
            logging.getLevelName(logging.ERROR):    "red",
            logging.getLevelName(logging.CRITICAL): "bold_red",
        })
    else:
        formatter = logging.Formatter(format, date_format)

    if to_stderr:
        handler = logging.StreamHandler()
    else:
        class _JournaldLogHandler(JournaldLogHandler):
            def emit(self, record):
                call_started = time.monotonic()
                call_result = super().emit(record)
                call_time = time.monotonic() - call_started
                if call_time > _MAX_HANDLER_CALL_DURATION:
                    msg_size = sys.getsizeof(record.getMessage(), 0)
                    print("Journald handler call took {}s (message size: {}).".format(call_time, msg_size), file=sys.stderr)
                return call_result

        handler = _JournaldLogHandler()

    handler.setFormatter(formatter)

    root_logger = logging.getLogger()

    if names is None:
        loggers = [root_logger]
    else:
        loggers = [logging.getLogger(name) for name in names]

        # `logger.disabled = True` doesn't work here because disabled flag is checked only for logger for which the log
        # message has been created and not checked when the message has been propagated from a child logger.
        root_logger.setLevel(logging.CRITICAL + 1)
        root_logger.addHandler(logging.NullHandler())
        logging.lastResort = None

    for logger in loggers:
        logger.setLevel(level)
        logger.addHandler(handler)

    if fallback_to_stderr:
        log.critical("Failed to load cysystemd.journal module. Falling back to stderr.")


def alter_caller(call_depth):
    """May be used as log.info(..., **alter_caller(...)) to alter the log caller."""

    if call_depth <= 0:
        return {}

    frame = inspect.currentframe().f_back
    for depth in range(call_depth):
        frame = frame.f_back

    filename, line_number, function_name, *_ = inspect.getframeinfo(frame)

    return {"extra": {"real_caller": (filename, line_number, function_name)}}


def mask_sensitive_fields(json_data, extra_fields=None):
    """Masking fields in-place. Note: nested fields are not masked."""

    if not isinstance(json_data, dict):
        log.warning("Masking failed: json_data is not dict.")
        return json_data

    result = json_data.copy()

    fields_to_mask = _MASKED_FIELDS
    if extra_fields is not None:
        fields_to_mask += (extra_fields,) if isinstance(extra_fields, str) else tuple(extra_fields)

    for field in fields_to_mask:
        if field in result:
            result[field] = _MASKING_VALUE

    return result
