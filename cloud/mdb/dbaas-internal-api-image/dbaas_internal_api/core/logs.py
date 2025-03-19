# -*- coding: utf-8 -*-
"""
DBaaS Internal API Logging commons
"""

import collections.abc
import json
import logging
import logging.config

SKIP_FIELDS = [
    'msg',
    'args',
    'exc_info',
    'exc_text',
    'filename',
    'processName',
    'relativeCreated',
    'pathname',
    'funcName',
    'module',
    'name',
    'stack_info',
    'lineno',
]

MASK_FIELDS = [
    'password',
]


def mask_dict(data):
    """
    Return copy of input dict with masked fields
    """
    ret = dict()
    for key, value in data.items():
        if key in MASK_FIELDS:
            ret[key] = '***'
        elif isinstance(value, collections.abc.Mapping):
            ret[key] = mask_dict(value)
        elif isinstance(value, collections.abc.Iterable) and not isinstance(value, str):
            ret[key] = []
            for element in value:
                if isinstance(element, collections.abc.Mapping):
                    ret[key].append(mask_dict(element))
                else:
                    ret[key].append(element)
        else:
            ret[key] = value

    return ret


def mask_fields(val):
    """
    Hide passwords and so on in request data
    """
    try:
        return mask_dict(json.loads(val.data.decode('utf-8'))) if val.data else None
    except json.JSONDecodeError as exc:
        return '{exc!r}'.format(exc=exc)


def tskv_escape(val):
    """
    Properly escape value for tskv
    """
    try:
        if isinstance(val, int):
            val = str(val)
        elif isinstance(val, float):
            val = '%.6f' % val
        elif isinstance(val, collections.abc.Mapping):
            val = json.dumps(val)
        elif val is None:
            val = ''

        return (
            val.replace('\\', '\\\\')
            .replace('\t', '\\t')
            .replace('\n', '\\n')
            .replace('\r', '\\r')
            .replace('\0', '\\0')
            .replace('=', '\\=')
        )
    except Exception:
        return tskv_escape(repr(val))


def prepare(data):
    """
    Mangle fields a bit (drop unused, rename some fields)
    """
    ret = []
    for key, val in data:
        if key == 'created':
            ret.append(('unixtime', int(val)))
        elif key == 'msecs':
            ret.append(('ms', val))
        elif key == 'request':
            ret.append(
                (
                    'request_id',
                    val.environ.get('HTTP_X_REQUEST_ID', None),
                )
            )
            ret.append(
                (
                    'real_ip',
                    val.environ.get('HTTP_X_REAL_IP', None),
                )
            )
            if getattr(val, 'auth_context', None):
                for context_key, context_value in val.auth_context.items():
                    if context_key not in ['token', 'provider']:
                        ret.append((context_key, context_value))
            ret.append(('endpoint', val.endpoint))
            ret.append(('method', val.method))
            ret.append(('user_agent', str(val.user_agent)))
            ret.append(('request_body', mask_fields(val)))
        elif key == 'response':
            ret.append(('status_code', val.status_code))
            ret.append(('content_length', val.content_length))
        elif key not in SKIP_FIELDS:
            ret.append((key, val))

    return ret


class TSKVFormatter(logging.Formatter):
    """
    DBaaS Internal API TSKV formatter
    """

    def __init__(self, tskv_format, *args, **kwargs):
        self.tskv_format = tskv_format
        super(TSKVFormatter, self).__init__(*args, **kwargs)

    def format(self, record):
        """
        Format record to tskv
        """
        fields = list(vars(record))
        fields.sort()

        data = [(name, getattr(record, name, None)) for name in fields]
        data.append(('tskv_format', self.tskv_format))
        data.append(('message', record.getMessage()))
        data = prepare(data)

        ret = 'tskv\t%s' % ('\t'.join('%s=%s' % (tskv_escape(key), tskv_escape(val)) for key, val in data))

        return ret


def init_logging(app):
    """
    Set up logging
    """
    logging.getLogger('werkzeug').disabled = True
    logging.config.dictConfig(app.config['LOGCONFIG'])  # just once
