from logging.handlers import SysLogHandler
SysLogHandler.append_nul = False

LOGGING = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters': {
        'verbose': {'format': '%(asctime)s irrd[%(process)d]: [%(name)s#%(levelname)s] %(message)s'},
        'json': {
            'format': '%(asctime)s%(process)d%(name)s%(levelname)s%(message)s',
            'class': 'pythonjsonlogger.jsonlogger.JsonFormatter',
        }
    },
    'handlers': {
        # "File" handler which writes messages to a file.
        # Note that the "file" key is arbitrary, you can
        # create ones like "file1", "file2", if you want
        # multiple handlers for different paths.
        'file': {
            'class': 'logging.handlers.WatchedFileHandler',
            'filename': '/var/log/irrd/irrd.log',
            'formatter': 'verbose',
        },
        'syslog': {
            'class': 'logging.handlers.SysLogHandler',
            'address': ('localhost', 10514),
            'facility': "local0",
            'formatter': 'json',
        },
    },
    'loggers': {
        # Tune down some very loud and not very useful loggers
        # from libraries. Propagation is the default, which means
        # loggers discard messages below their level, and then the
        # remaining messages are passed on, eventually reaching
        # the actual IRRd logger.
        'passlib.registry': {
            'level': 'INFO',
        },
        'gnupg': {
            'level': 'INFO',
        },
        'sqlalchemy': {
            'level': 'WARNING',
        },
        # Actual IRRd logging feature, passing the log message
        # to the "file" handler defined above.
        '': {
            'handlers': ['file', 'syslog'],
            'level': 'INFO',
        },
    },
}
