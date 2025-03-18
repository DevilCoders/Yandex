import time
import logging

import six
import ujson


class DeployFormatter(logging.Formatter):
    COMMON_FIELDS = (
        ('levelname', 'levelStr'),
        ('levelno', 'level'),
        ('asctime', '@timestamp'),
        ('name', 'loggerName'),
        ('threadName', 'threadName'),
        ('message', 'message'),
    )

    COMMON_CONTEXT_FIELDS = (
        ('process', 'pid'),
        ('processName', 'process'),
        ('thread', 'thread_id'),
        ('funcName', 'func_name'),
        ('lineno', 'lineno'),
        ('module', 'module'),
        ('pathname', 'filename'),
    )

    KNOWN_KEYS = {
        'name', 'msg', 'message', 'args', 'levelname',
        'levelno', 'pathname', 'filename', 'module',
        'exc_info', 'exc_text', 'lineno', 'funcName',
        'created', 'msecs', 'relativeCreated', 'thread',
        'threadName', 'processName', 'process', 'asctime',
    }

    def formatTime(self, record, datefmt=None):
        gt = time.gmtime(record.created)
        return time.strftime('%Y-%m-%dT%H:%M:%S.%%03dZ', gt) % (record.msecs,)

    def format(self, record):
        record.message = record.getMessage()
        record.asctime = self.formatTime(record)

        msg = {}
        for field, dest_name in self.COMMON_FIELDS:
            if hasattr(record, field):
                msg[dest_name] = getattr(record, field)

        msg['@fields'] = {}
        for field, dest_name in self.COMMON_CONTEXT_FIELDS:
            if hasattr(record, field):
                msg['@fields'][dest_name] = getattr(record, field)

        for k, v in six.iteritems(record.__dict__):
            if k not in self.KNOWN_KEYS:
                msg['@fields'][k] = v

        if record.exc_info and not record.exc_text:
            record.exc_text = self.formatException(record.exc_info)
        if record.exc_text:
            msg['stackTrace'] = record.exc_text

        return ujson.dumps(msg)
