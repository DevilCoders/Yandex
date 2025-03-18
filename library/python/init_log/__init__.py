import logging
from logging.handlers import RotatingFileHandler
import coloredlogs
import textwrap


def make_wrapping(formatter_cls, wrap_width):
    class Wrapping(formatter_cls):
        def format(self, *args, **kwargs):
            potentially_long_string = super(Wrapping, self).format(*args, **kwargs)
            return '\\\n'.join(textwrap.wrap(potentially_long_string, wrap_width, drop_whitespace=False))
    return Wrapping


def init_log(level='INFO', append_ts=False, colored=True, wrap_width=None, log_file=None,
             append_pid=False):
    logging.root.setLevel(getattr(logging, level))
    init_handler('DEBUG', append_ts, colored, wrap_width, log_file, append_pid)


def init_handler(level='INFO', append_ts=False, colored=True, wrap_width=None, log_file=None,
                 append_pid=False, rotate=False):
    class Flt(logging.Filter):
        def filter(self, record):
            record.name = record.name[:20]

            return True

    fmt = '%(name)-20s | %(levelname).1s | %(message)s'

    if append_ts:
        fmt = '%(asctime)s | ' + fmt

    if append_pid:
        fmt = '%(process)d | ' + fmt

    if log_file and rotate:
        console_log = RotatingFileHandler(log_file, maxBytes=10*2**20, backupCount=5)
    elif log_file:
        console_log = logging.FileHandler(log_file)
    else:
        console_log = logging.StreamHandler()
    console_log.setLevel(getattr(logging, level))
    if colored:
        formatter_cls = coloredlogs.ColoredFormatter
    else:
        formatter_cls = logging.Formatter
    if wrap_width:
        formatter_cls = make_wrapping(formatter_cls, wrap_width)
    console_log.setFormatter(formatter_cls(fmt))
    console_log.addFilter(Flt())

    logging.root.addHandler(console_log)
