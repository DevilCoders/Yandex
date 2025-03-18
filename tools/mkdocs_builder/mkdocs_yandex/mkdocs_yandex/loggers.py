import logging

from mkdocs.utils import warning_filter

try:
    from mkdocs.utils import CountHandler
    warning_counter = CountHandler()
    warning_counter.setLevel(logging.WARNING)
except ImportError:
    warning_counter = None


def get_logger(name):
    logger = logging.getLogger("mkdocs.plugins.{}".format(name))
    logger.addFilter(warning_filter)
    if warning_counter:
        logger.addHandler(warning_counter)
    return logger


def get_warning_counts():
    if warning_counter:
        return sum(row[-1] for row in warning_counter.get_counts())
    else:
        return warning_filter.count
