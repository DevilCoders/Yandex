import datetime
import json
from logging import LogRecord

import sys
import time

from cloud.mdb.internal.python.logs.format.json import JsonFormatter


def test_logs_to_json(mocker):
    try:
        raise ValueError('for tests')
    except ValueError:
        tb = sys.exc_info()
    formatter = JsonFormatter()
    mocker.patch(
        'logging.time.time', lambda: time.mktime(datetime.datetime.strptime('01/12/2011', '%d/%m/%Y').timetuple())
    )
    record = LogRecord(
        name='test_name',
        level=20,
        pathname='test/path.py',
        lineno=100500,
        msg='hello, %s',
        args=('username',),
        exc_info=tb,
        func='test_func_name',
    )
    record.extra_field = 'mdb'
    result = json.loads(formatter.format(record))
    expectations = {
        'stackTrace': 'Traceback (most recent call last):\n  File '
        '"cloud/mdb/internal/python/test/logs/format/json_test.py", line 13, in test_logs_to_json\n    '
        'raise ValueError(\'for tests\')\nValueError: for tests',
        'name': 'test_name',
        'msg': 'hello, username',
        'level': 'INFO',
        'levelno': 20,
        'pathname': 'test/path.py',
        'filename': 'path.py',
        'module': 'path',
        'lineno': 100500,
        'funcName': 'test_func_name',
        'threadName': 'MainThread',
        'processName': 'MainProcess',
        'asctime': '2011-12-01T00:00:00',
        'orig_msg': 'hello, %s',
        'extra_field': 'mdb',
    }
    assert expectations == result
