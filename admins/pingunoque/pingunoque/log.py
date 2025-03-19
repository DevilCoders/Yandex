"""pingunoque log"""

import logging
import logging.handlers as handlers
from .config import load_config

LOGLEVELS = {'CRITICAL': 50, 'FATAL': 50, 'ERROR': 40, 'WARNING': 30,
             'WARN': 30, 'INFO': 20, 'DEBUG': 10, 'TRACE': 5}
TRACE = LOGLEVELS['TRACE']
logging.addLevelName(TRACE, "TRACE")
LOG = logging.getLogger()

# pylint: disable=invalid-name
trace = lambda *args, **kwargs: LOG.log(TRACE, *args, **kwargs)
debug = LOG.debug
info = LOG.info
warn = LOG.warning
error = LOG.error
exception = LOG.exception

if not getattr(globals(), "LOG", False):
    CONFIG = load_config()

    LOG_HANDLER = handlers.WatchedFileHandler(CONFIG['log_file'])
    LOG_HANDLER.setFormatter(logging.Formatter(
        "%(asctime)s [%(levelname)-6s] %(threadName)s - "+\
                "%(module)s.%(funcName)s:%(lineno)d: %(message)s"
    ))
    LOG.setLevel(LOGLEVELS[CONFIG['log_level'].replace("logging.", "")])
    LOG.addHandler(LOG_HANDLER)
    trace("Logger initialized, daemon config: %s", CONFIG)
