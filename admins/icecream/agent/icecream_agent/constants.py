""" icecream-agent constants """

CONFIG_DIR = '/etc/yandex/icecream-agent/'
CONFIG_PATH = CONFIG_DIR + 'config.json'
TMP_DIR = '/var/cache/icecream-agent'

ENABLED_APIS = [
    './icecream_agent/api/v1/spec.yaml'
]

DEFAULT_CONFIG = {
    'log': {
        'level': 'DEBUG'
    },
    'storage': {
        'type': 'lvm',
        'name': 'lxd'
    },
    'api': {
        'host': 'https://i.yandex-team.ru'
    }
}

LOG_DIR = '/var/log/yandex/icecream-agent/'
API_LOG_PATH = LOG_DIR + 'api.log'
MULE_LOG_PATH = LOG_DIR + 'mule.log'
LOG_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - " \
             "%(filename)s - %(funcName)s - %(lineno)d - %(message)s"
LOG_DATE_FORMAT = "%Y-%m-%d %H:%M:%S"
LOG_CONFIG = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters': {
        'standard': {
            'format': LOG_FORMAT,
            'datefmt': LOG_DATE_FORMAT,
        }
    },
    'handlers': {
        'console': {
            'level': 'DEBUG',
            'formatter': 'standard',
            'class': 'logging.StreamHandler',
        },
        'api_log': {
            'level': 'DEBUG',
            'formatter': 'standard',
            'class': 'logging.handlers.WatchedFileHandler',
            'filename': API_LOG_PATH,
            'encoding': 'utf8'
        },
        'mule_log': {
            'level': 'DEBUG',
            'formatter': 'standard',
            'class': 'logging.handlers.WatchedFileHandler',
            'filename': MULE_LOG_PATH,
            'encoding': 'utf8'
        }
    },
    'loggers': {
        'api': {
            'handlers': ['api_log'],
            'level': 'DEBUG',
        },
        'mule': {
            'handlers': ['mule_log'],
            'level': 'DEBUG'
        }
    }
}
