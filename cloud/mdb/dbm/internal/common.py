# -*- encoding: utf-8
"""
Common functions
"""

import logging
import logging.config

from configparser import RawConfigParser

from flask import abort, jsonify

from .config import app_config


def parse_bool(value):
    """
    Parse boolean in query string
    """
    return RawConfigParser.BOOLEAN_STATES.get(value.lower(), False)


def parse_int(value):
    """
    Parse int in query string
    """
    try:
        return int(value)
    except ValueError:
        return 0


def parse_float(value):
    """
    Parse float in query string
    """
    try:
        return float(value)
    except ValueError:
        return 0


def parse_query_string(query_string, valid_keys):
    """
    Parse query string into dict
    """
    pairs = query_string.split(';')
    ret = dict()
    for pair in pairs:
        if not pair:
            continue
        key, value = pair.split('=')
        if key not in valid_keys:
            res = jsonify({'error': f'Unknown key: {key}'})
            res.status_code = 400
            abort(res)
        ret[key] = valid_keys[key](value)

    return ret


def init_logging():
    """
    Initialize loggers
    """
    default = {
        'version': 1,
        'disable_existing_loggers': True,
        'formatters': {
            'tskv': {
                '()': 'cloud.mdb.dbm.internal.log.TSKVFormatter',
                'tskv_format': 'db-maintenance',
            },
        },
        'handlers': {
            'tskv': {
                'class': 'logging.handlers.RotatingFileHandler',
                'level': 'DEBUG',
                'formatter': 'tskv',
                'maxBytes': 10485760,
                'filename': '/var/log/db-maintenance/dbm.log',
                'backupCount': 10,
            },
        },
        'loggers': {
            'flask.app': {
                'handlers': ['tskv'],
                'level': 'DEBUG',
            },
        },
    }
    config = app_config()
    default.update(config.get('LOGCONFIG', {}))
    logging.getLogger('werkzeug').disabled = True
    logging.config.dictConfig(default)
