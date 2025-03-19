import yatest.common

from cloud.mdb.internal.python.pytest.utils import parametrize
from test_utils import (
    ShutdownSuccessConn,
    ShutdownFailedConn,
    ShutdownUnexpectedConn,
    DeadConn,
    main_test,
    get_conn_sequence,
    SUCCESS_RESULT_CODE,
    SUCCESS_RESULT_PRINT,
    ERROR_RESULT_CODE,
    WAIT_CMD_ERROR_PARTS,
    UNEXPECTED_RESULT,
    ShutdownSuccessLoadingConn,
    ShutdownSuccessAofFIlledConn,
)


def no_pids(*args, **kwrags):
    return []


def custom_pids(*args, **kwrags):
    return [12345]


TEST_DATA = [
    {
        'id': 'Redis stopped',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'stop_and_wait',
            ],
            'conn_func': get_conn_sequence(DeadConn, DeadConn),
            'custom_patch_dict': {'commands.stop_and_wait.get_redis_pids': custom_pids},
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': WAIT_CMD_ERROR_PARTS + [UNEXPECTED_RESULT],
        },
    },
    {
        'id': 'Redis stop failed, then succeeded',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'stop_and_wait',
            ],
            'conn_func': get_conn_sequence(ShutdownFailedConn, ShutdownSuccessConn),
            'custom_patch_dict': {'commands.stop_and_wait.get_redis_pids': custom_pids},
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Redis stop unexpectedly failed',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'stop_and_wait',
            ],
            'conn_func': get_conn_sequence(ShutdownFailedConn, ShutdownUnexpectedConn),
            'custom_patch_dict': {'commands.stop_and_wait.get_redis_pids': custom_pids},
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': WAIT_CMD_ERROR_PARTS + ['unexpected result - connection is not dropped: None'],
        },
    },
    {
        'id': 'Redis started, then stopped',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'stop_and_wait',
            ],
            'conn_func': get_conn_sequence(ShutdownSuccessConn, DeadConn),
            'custom_patch_dict': {'commands.stop_and_wait.get_redis_pids': custom_pids},
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Redis loading, then stopped',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'stop_and_wait',
            ],
            'conn_func': get_conn_sequence(ShutdownSuccessLoadingConn, DeadConn),
            'custom_patch_dict': {'commands.stop_and_wait.get_redis_pids': custom_pids},
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Redis AOF full, then stopped',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'stop_and_wait',
            ],
            'conn_func': get_conn_sequence(ShutdownSuccessAofFIlledConn, DeadConn),
            'custom_patch_dict': {'commands.stop_and_wait.get_redis_pids': custom_pids},
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Redis stopped already, nothing done',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'stop_and_wait',
            ],
            'conn_func': [],
            'custom_patch_dict': {'commands.stop_and_wait.get_redis_pids': no_pids},
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
]


@parametrize(*TEST_DATA)
def test_redisctl_stop_and_wait(
    capsys, argv, conn_func, custom_patch_dict, expected_code, expected_out_parts, expected_err_parts
):
    main_test(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts, custom_patch_dict)
