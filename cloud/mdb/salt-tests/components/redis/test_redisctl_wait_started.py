import yatest.common

from cloud.mdb.internal.python.pytest.utils import parametrize
from test_utils import (
    OkConn,
    DeadConn,
    main_test,
    get_conn_sequence,
    WAIT_CMD_ERROR_PARTS,
    SUCCESS_RESULT_CODE,
    ERROR_RESULT_CODE,
)

import utils


WAIT_STARTED = "wait started"
TEST_DATA = [
    {
        'id': 'Redis is already started',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_started',
            ],
            'conn_func': get_conn_sequence(OkConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [utils.state.RedisStatus.READY_TO_ACCEPT_COMMANDS.value],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Redis is dead',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_started',
            ],
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': WAIT_CMD_ERROR_PARTS + [WAIT_STARTED],
        },
    },
    {
        'id': 'Redis dead, then started',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_started',
            ],
            'conn_func': get_conn_sequence(DeadConn, OkConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [utils.state.RedisStatus.READY_TO_ACCEPT_COMMANDS.value],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Redis dead more than restarts count',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_started',
            ],
            'conn_func': get_conn_sequence(DeadConn, DeadConn, OkConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': WAIT_CMD_ERROR_PARTS + [WAIT_STARTED],
        },
    },
]


@parametrize(*TEST_DATA)
def test_redisctl_wait_started(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts):
    main_test(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts)
