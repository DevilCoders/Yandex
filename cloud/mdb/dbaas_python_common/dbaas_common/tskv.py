# -*- coding: utf-8 -*-
"""
DBaaS Logging commons
"""

import collections.abc
import json
import logging

TRANS_TABLE = {
    ord('\\'): '\\\\',
    ord('\t'): '\\t',
    ord('\n'): '\\n',
    ord('\r'): '\\r',
    ord('\0'): '\\0',
    ord('='): '\\=',
}


def tskv_escape(val):
    """
    Properly escape value for tskv
    """
    if val is None:
        return ''
    if isinstance(val, int):
        return str(val)
    if isinstance(val, float):
        return '{value:.6}'.format(value=val)

    try:
        if isinstance(val, collections.abc.Mapping):
            val = json.dumps(val)

        return val.translate(TRANS_TABLE)
    except Exception:
        return tskv_escape(repr(val))


class TSKVFormatter(logging.Formatter):
    """
    DBaaS TSKV formatter
    """

    def __init__(self, tskv_format, *args, only_fields=None, **kwargs):
        self.tskv_format = tskv_format
        self.only_fields = only_fields
        super(TSKVFormatter, self).__init__(*args, **kwargs)

    # pylint: disable=no-self-use
    def prepare(self, data):
        """
        Mangle fields a bit according to fields map
        """
        ret = []
        for key, val in data:
            if key == 'created':
                ret.append(('unixtime', int(val)))
            elif key == 'msecs':
                ret.append(('ms', val))
            elif key != 'msg':
                ret.append((key, val))

        return ret

    def format(self, record):
        """
        Format record to tskv
        """
        fields = list(vars(record))
        fields.sort()

        data = [(name, getattr(record, name, None)) for name in fields]
        data.append(('tskv_format', self.tskv_format))
        data.append(('message', record.getMessage()))
        data = self.prepare(data)

        ret = 'tskv\t{log}'.format(
            log=(
                '\t'.join(
                    '{key}={value}'.format(key=tskv_escape(key), value=tskv_escape(val))
                    for key, val in data
                    if not self.only_fields or key in self.only_fields
                )
            )
        )

        return ret
