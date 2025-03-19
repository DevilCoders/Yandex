import yatest.common

from cloud.mdb.internal.python.pytest.utils import parametrize
from test_utils import (
    DeadConn,
    OkConn,
    main_test,
    get_conn_sequence,
    SUCCESS_RESULT_CODE,
    ERROR_RESULT_CODE,
    UNEXPECTED_RESULT,
)

import utils


TEST_DATA = [
    {
        'id': 'Redis is alive',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'is_alive',
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
                'is_alive',
            ],
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': [UNEXPECTED_RESULT],
        },
    },
]


@parametrize(*TEST_DATA)
def test_redisctl_is_alive(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts):
    main_test(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts)
