import os
import logging

import library.python.json as json


LOG_FORMAT = (
    '%(asctime)s\t'
    '%(process)-6d\t'
    '%(threadName)-10s\t'
    '%(funcName)-30s\t'
    '%(name)-30s\t'
    '%(levelname)-8s\t'
    '%(message)s'
)


class JsonFormatter(logging.Formatter):
    def __init__(self):
        super(JsonFormatter, self).__init__()

    def format(self, record):
        result = {
            "timestamp": record.created,
            "msg": record.getMessage(),
            "levelStr": record.levelname,
            "loggerName": record.name,
            "@fields": {
                "threadName": record.processName if record.processName != 'MainProcess' else record.threadName,
                "pid": record.process,
            },
            "uptime_milliseconds": record.relativeCreated,
        }

        if record.exc_info:
            result['stackTrace'] = self.formatException(record.exc_info)

        if record.args and isinstance(record.args, dict):
            result['@fields'].update(record.args)

        return json.dumps(result, ensure_ascii=False)


class LogConfig:
    def __init__(
        self,
        app_name,
        config,
    ):
        self.app_name = app_name
        self.config = config

    @property
    def log_path(self):
        if self.config.get('path'):
            return self.config.path
        else:
            return '/var/log/{}'.format(self.app_name)

    @property
    def log_filename(self):
        if self.config.get('filename'):
            return self.config.filename
        else:
            return '{}.log'.format(self.app_name)

    @property
    def log_file(self):
        if self.config.get('file'):
            return self.config.file
        else:
            return os.path.join(self.log_path, self.log_filename)

    @property
    def logger_handlers(self):
        handlers = [self.app_name]
        if self.config.get('stdout'):
            handlers.append('stdout')

        return handlers

    @property
    def handlers(self):
        handlers = {}
        handlers[self.app_name] = {
            'level': self.config.get('level', 'DEBUG'),
            'class': 'logging.handlers.RotatingFileHandler',
            'filename': self.log_file,
            'maxBytes': 52428800,
            'backupCount': 4,
            'formatter': 'standard',
            'filters': [],
        }
        if self.config.get('stdout'):
            handlers['stdout'] = {
                'level': self.config.get('level', 'DEBUG'),
                'class': 'logging.StreamHandler',
                'formatter': 'standard',
                'filters': [],
            }
        return handlers

    @property
    def formatters(self):
        formatters = {}
        if self.config.get('json'):
            formatters['standard'] = {
                '()': JsonFormatter,
            }
        else:
            formatters['standard'] = {
                'format': LOG_FORMAT,
            }
        return formatters

    @property
    def filters(self):
        return {}

    @property
    def loggers(self):
        return {
            'django': {
                'handlers': self.logger_handlers,
                'level': self.config.get('django_level', 'DEBUG'),
                'propagate': False
            },
            'django.request': {
                'handlers': self.logger_handlers,
                'level': self.config.get('django_request_level', 'DEBUG'),
                'propagate': False
            },
            'django.template': {
                'handlers': self.logger_handlers,
                'level': self.config.get('django_template_level', 'INFO'),
                'propagate': False
            },
            self.app_name: {
                'handlers': self.logger_handlers,
                'level': self.config.get('level', 'DEBUG'),
                'propagate': False
            },
            '': {
                'handlers': self.logger_handlers,
                'level': self.config.get('all', 'DEBUG'),
                'propagate': False
            },
        }

    def as_dict(self):
        return {
            'version': 1,
            'disable_existing_loggers': False,
            'filters': self.filters,
            'formatters': self.formatters,
            'handlers': self.handlers,
            'loggers': self.loggers,
        }
