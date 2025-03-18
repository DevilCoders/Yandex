import json
import time
import socket
import logging
from sys import stdout
from logging.handlers import WatchedFileHandler

HOSTNAME = socket.gethostname()


class JSONFormatter(logging.Formatter):

    @classmethod
    def format_timestamp(cls, record):
        return time.strftime("%Y-%m-%dT%H:%M:%S.{:03.0f}Z", time.gmtime(record.created)).format(record.msecs % 1000)

    def format(self, record, serialize=True):
        r = {
            '@timestamp': self.format_timestamp(record),
            'message': record.getMessage(),
            'host': HOSTNAME,
        }
        if record.exc_info:
            r.update({'exception': self.formatException(record.exc_info)})
        elif record.msg:
            r.update({'message': record.msg % record.args if record.args else record.msg})
        r.update({'level': record.levelname, 'filename': "{}:{}".format(record.filename, record.lineno)})
        r.update(getattr(record, 'extra', {}))
        return json.dumps(r)


Base = logging.getLoggerClass()


class JSLogger(Base):
    def __init__(self, name, logs_path=None, level=logging.INFO, **additional_args):
        Base.__init__(self, name)
        formatter = JSONFormatter()
        if logs_path is not None:
            handler = WatchedFileHandler(logs_path)
            handler.setFormatter(formatter)
            self.addHandler(handler)
        handler = logging.StreamHandler(stream=stdout)
        handler.setFormatter(formatter)
        self.addHandler(handler)
        self.setLevel(level)
        self.additional_args = additional_args

    def makeRecord(self, name, level, fn, lno, msg, args, exc_info, func=None, extra=None):
        record = logging.LogRecord(name, level, fn, lno, msg, args, exc_info, func)
        record.extra = extra
        return record

    def _log(self, level, msg, args, exc_info=None, **kwargs):
        res = dict()
        res.update(self.additional_args)
        res.update(**kwargs)
        return Base._log(self, level, msg, args, exc_info, extra=res)
