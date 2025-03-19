from logging.config import dictConfig


def conf_logging():
    dictConfig(
        {
            'version': 1,
            'disable_existing_loggers': True,
            'formatters': {
                'default': {
                    'class': 'logging.Formatter',
                    'datefmt': '%Y-%m-%d %H:%M:%S',
                    'format': '%(asctime)s %(name)-15s %(levelname)-10s %(message)s',
                },
            },
            'handlers': {
                'streamhandler': {
                    'level': 'DEBUG',
                    'class': 'logging.StreamHandler',
                    'formatter': 'default',
                    'stream': 'ext://sys.stdout',
                },
                'null': {
                    'level': 'DEBUG',
                    'class': 'logging.NullHandler',
                },
            },
            'loggers': {
                '': {
                    'handlers': [
                        'streamhandler',
                    ],
                    'level': 'DEBUG',
                },
            },
            'root': {'level': 'DEBUG', 'handlers': ['streamhandler']},
        }
    )
