"""
Logging utils
"""

import logging
import pprint
from typing import Any

from .config import get_background_logger_name


def get_logger() -> logging.Logger:
    """
    Return logger
    """
    return logging.getLogger(get_background_logger_name())


def log_warn(msg: str, *args: Any, **kwargs: Any) -> None:
    """
    log warning
    """
    get_logger().warning(msg, *args, **kwargs)


def log_debug_struct(*args):
    """
    Log with pprint
    """
    get_logger().debug(pprint.pformat(args))
