import yatest.common

from cloud.mdb.internal.python.pytest.utils import parametrize
from test_utils import (
    AofEnabledConn,
    DeadConn,
    DumpConn,
    PersOffConn,
    RDBEnabledConn,
    main_test,
    get_conn_sequence,
    ERROR_RESULT_CODE,
    SUCCESS_RESULT_CODE,
    SUCCESS_RESULT_PRINT,
    OkConn,
)

TEST_DATA = [
    {
        'id': 'Dead Redis, no save',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'run_save_when_persistence_off',
            ],
            'conn_func': get_conn_sequence(DeadConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': ['Redis is not ready for accept commands'],
        },
    },
    {
        'id': 'Alive Redis, aof on, no save',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'run_save_when_persistence_off',
            ],
            'conn_func': get_conn_sequence(OkConn, AofEnabledConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': ['AOF or RDB is on, skipping bgsave'],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Alive Redis, rdb on, no save',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'run_save_when_persistence_off',
            ],
            'conn_func': get_conn_sequence(OkConn, RDBEnabledConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': ['AOF or RDB is on, skipping bgsave'],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Alive Redis, aof off, rdb off - save ok',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'run_save_when_persistence_off',
            ],
            'conn_func': get_conn_sequence(DumpConn, PersOffConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [SUCCESS_RESULT_PRINT],
            'expected_err_parts': [],
        },
    },
]


@parametrize(*TEST_DATA)
def test_redisctl_get_config_options(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts):
    main_test(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts)
