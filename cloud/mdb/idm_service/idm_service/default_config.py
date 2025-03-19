"""Default config. See: flask_appconfig."""
CONNSTRINGS = ['host=localhost port=6432']

HIDE_FLAG = '/tmp/.mdb-idm-service-hide'  # nosec

EAGER_MODIFY = True

JSON_AS_ASCII = False

PASS_LIFESPAN_DAYS = 90

SMTP_HOST = 'outbound-relay.yandex.net'

CRYPTO = {
    'client_pub_key': 'cli_pub_key',
    'api_sec_key': 'api_sec_key',
}

LOGCONFIG = {
    'version': 1,
    'disable_existing_loggers': True,
    'formatters': {
        'standard': {
            'format': '%(asctime)s %(levelname)-8s %(name)-10s: %(message)s',
        },
    },
    'handlers': {
        'console': {
            'class': 'logging.StreamHandler',
            'formatter': 'standard',
            'level': 'DEBUG',
            'stream': 'ext://sys.stdout',
        },
    },
    'loggers': {
        'idm_service': {
            'level': 'DEBUG',
            'handlers': ['console'],
        },
    },
}
