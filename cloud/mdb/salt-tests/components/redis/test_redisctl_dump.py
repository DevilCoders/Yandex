import os
import tempfile
from collections import namedtuple

import yatest.common
from hamcrest import assert_that, equal_to

from cloud.mdb.internal.python.pytest.utils import parametrize
from test_utils import (
    DeadConn,
    main_test,
    get_conn_sequence,
    SUCCESS_RESULT_CODE,
    ERROR_RESULT_CODE,
    DumpConn,
    TEST_DUMP_DATA,
    BrokenLastsaveDumpConn,
    DumpInProgressConn,
    SUCCESS_RESULT_PRINT,
    DumpErrorConn,
)
from utils.file_ops import STDOUT_PATH


def get_path_from_argv(argv):
    for i, arg in enumerate(argv):
        if arg == '--file':
            return argv[i + 1]


def get_tmp_file_name():
    with tempfile.NamedTemporaryFile(delete=False) as f:
        return f.name


def clean_tmp_file(path):
    if path and path != STDOUT_PATH and os.path.exists(path):
        os.unlink(path)


DumpData = namedtuple('DumpData', 'is_tmp_path expected clean')
DEFAULT_DUMP_DATA = DumpData(is_tmp_path=True, expected=None, clean=True)
TEST_DATA = [
    {
        'id': 'Dead Redis, no dump',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--file',
                '-',
            ],
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ['Redis is not ready for accept commands'],
            'dump_data': DEFAULT_DUMP_DATA,
        },
    },
    {
        'id': 'Broken lastsave, no dump',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--file',
                '-',
            ],
            'conn_func': get_conn_sequence(BrokenLastsaveDumpConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ['broken lastsave'],
            'dump_data': DEFAULT_DUMP_DATA,
        },
    },
    {
        'id': 'Dump to STDOUT',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--bgsave-wait',
                '2',
                '--file',
                STDOUT_PATH,
            ],
            'conn_func': get_conn_sequence(DumpConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': [],
            'dump_data': DumpData(is_tmp_path=False, expected=TEST_DUMP_DATA, clean=True),
        },
    },
    {
        'id': 'Dump to -',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--bgsave-wait',
                '2',
                '--file',
                '-',
            ],
            'conn_func': get_conn_sequence(DumpConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': [],
            'dump_data': DumpData(is_tmp_path=False, expected=TEST_DUMP_DATA, clean=True),
        },
    },
    {
        'id': 'Dump to file',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--bgsave-wait',
                '2',
                '--file',
                get_tmp_file_name(),
            ],
            'conn_func': get_conn_sequence(DumpConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
            'dump_data': DumpData(is_tmp_path=True, expected=TEST_DUMP_DATA, clean=True),
        },
    },
    {
        'id': 'Dump to existing file, no dump',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--bgsave-wait',
                '2',
                '--file',
                get_tmp_file_name(),
            ],
            'conn_func': get_conn_sequence(DumpConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ['does already exists'],
            'dump_data': DumpData(is_tmp_path=True, expected=None, clean=False),
        },
    },
    {
        'id': 'Dump to file in progress',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--bgsave-wait',
                '2',
                '--file',
                get_tmp_file_name(),
            ],
            'conn_func': get_conn_sequence(DumpInProgressConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
            'dump_data': DumpData(is_tmp_path=True, expected=TEST_DUMP_DATA, clean=True),
        },
    },
    {
        'id': 'Dump to file error, too long, no dump',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--bgsave-wait',
                '2',
                '--file',
                get_tmp_file_name(),
            ],
            'conn_func': get_conn_sequence(DumpErrorConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ['Waiting for bgsave success call took too long'],
            'dump_data': DEFAULT_DUMP_DATA,
        },
    },
    {
        'id': 'Dump to file error, no timeout, no dump',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'dump',
                '--bgsave-wait',
                '7',
                '--file',
                get_tmp_file_name(),
            ],
            'conn_func': get_conn_sequence(DumpErrorConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ['unexpected_bgsave_error'],
            'dump_data': DEFAULT_DUMP_DATA,
        },
    },
]


@parametrize(*TEST_DATA)
def test_redisctl_get_config_options(
    capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts, dump_data
):
    is_tmp_path, expected, clean = dump_data
    path = get_path_from_argv(argv) if is_tmp_path else STDOUT_PATH
    if clean:
        clean_tmp_file(path)
    if not clean and path and path != STDOUT_PATH and not os.path.exists(path):
        open(path, 'w').close()

    main_test(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts)

    if not expected:
        clean_tmp_file(path)
        return

    with open(path, 'rb') as fobj:
        actual = fobj.read()
    assert_that(actual, equal_to(expected))
    clean_tmp_file(path)
