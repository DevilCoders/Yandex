import time
from sys import stdout
import logging
from logging.handlers import WatchedFileHandler
from contextlib import contextmanager
from json import dumps

from antiadblock.cryprox.cryprox.config.service import HOSTNAME, LOCAL_RUN, LOG_FILE

Base = logging.getLoggerClass()


class JSFormatter(logging.Formatter):
    def formatTime(self, record, fmt="%Y-%m-%dT%H:%M:%S.{:03.0f}Z"):
        return time.strftime(fmt, time.gmtime(record.created)).format(record.msecs % 1000)  # Because sometimes THERE IS A GOD-DAMNED 1000 of msecs

    def format(self, record):
        r = {'@timestamp': self.formatTime(record),
             'level': record.levelname,
             'timestamp': int(record.created * 1000),
             'hostname': HOSTNAME
             }
        if record.exc_info:
            r.update({'exception': self.formatException(record.exc_info)})
        elif record.msg:
            r.update({'message': record.msg % record.args if record.args else record.msg})
        if record.levelname == 'DEBUG':
            r.update({'level': record.levelname, 'filename': "{}:{}".format(record.filename, record.lineno)})
        r.update(getattr(record, 'extra', {}))
        try:
            return dumps(r)
        except Exception as e:
            return dumps({'message': 'Error trying to log, {}'.format(str(e)[:10000])})


class JSLogger(Base):
    def __init__(self, name, request_tags=(), log_file=LOG_FILE, **additional_args):
        Base.__init__(self, name)
        handler = logging.StreamHandler(stream=stdout) if LOCAL_RUN else WatchedFileHandler(log_file)
        handler.setFormatter(JSFormatter())
        self.addHandler(handler)
        self.additional_args = additional_args
        self.request_tags = request_tags

    def makeRecord(self, name, level, fn, lno, msg, args, exc_info, func=None, extra=None):
        record = logging.LogRecord(name, level, fn, lno, msg, args, exc_info, func)
        record.extra = extra
        return record

    def _log(self, level, msg, args, exc_info=None, **kwargs):
        res = {'request_tags': self.request_tags}
        res.update(self.additional_args)
        res.update(**kwargs)
        if self.getEffectiveLevel() <= logging.DEBUG:
            level = logging.DEBUG
        return Base._log(self, level, msg, args, exc_info, extra=res)

    @contextmanager
    def add_to_context(self, **args):
        """
        it was a logger context some days . But not any more. Because we lose context on async ioloops calls
        :param args:
        :return:
        """
        self.additional_args.update(args)
