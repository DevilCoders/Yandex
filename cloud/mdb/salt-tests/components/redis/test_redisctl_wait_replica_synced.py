import yatest.common

from cloud.mdb.internal.python.pytest.utils import parametrize
from test_utils import (
    main_test,
    get_conn_sequence,
    ReplicaSyncedConn,
    FailoverInProgressConn,
    MasterConn,
    ReplicaSyncInProgressConn,
    ReplicaUnexpectedStateConn,
    WAIT_CMD_ERROR_PARTS,
    SUCCESS_RESULT_CODE,
    ERROR_RESULT_CODE,
)


REPLICA_SYNCED = 'replica is not syncing now'
MASTER_NODE = "no need to wait for replica sync cause it's master"
SYNC_IN_PROGRESS = "replica sync in progress"
FAILOVER_IN_PROGRESS = "failover in progress"
REPLICA_UNEXPECTED = "replica sync state unexpected (see logs)"
TEST_DATA = [
    {
        'id': 'Replica is synced',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_replica_synced',
            ],
            'conn_func': get_conn_sequence(ReplicaSyncedConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [REPLICA_SYNCED],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Master node',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_replica_synced',
            ],
            'conn_func': get_conn_sequence(MasterConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [MASTER_NODE],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Replica sync in progress, then finished',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_replica_synced',
            ],
            'conn_func': get_conn_sequence(ReplicaSyncInProgressConn, ReplicaSyncedConn),
            'expected_code': SUCCESS_RESULT_CODE,
            'expected_out_parts': [REPLICA_SYNCED],
            'expected_err_parts': [],
        },
    },
    {
        'id': 'Replica sync in progress till restarts end',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_replica_synced',
            ],
            'conn_func': get_conn_sequence(ReplicaSyncInProgressConn, ReplicaSyncInProgressConn, ReplicaSyncedConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': WAIT_CMD_ERROR_PARTS + [SYNC_IN_PROGRESS],
        },
    },
    {
        'id': 'Failover in progress till restarts end',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_replica_synced',
            ],
            'conn_func': get_conn_sequence(ReplicaSyncInProgressConn, FailoverInProgressConn, ReplicaSyncedConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': WAIT_CMD_ERROR_PARTS + [FAILOVER_IN_PROGRESS],
        },
    },
    {
        'id': 'Unexpected state till restarts end',
        'args': {
            'argv': [
                '--redis-data-path',
                yatest.common.test_source_path('test_data/info.json_main'),
                '--action',
                'wait_replica_synced',
            ],
            'conn_func': get_conn_sequence(ReplicaSyncInProgressConn, ReplicaUnexpectedStateConn, ReplicaSyncedConn),
            'expected_code': ERROR_RESULT_CODE,
            'expected_out_parts': [],
            'expected_err_parts': WAIT_CMD_ERROR_PARTS + [REPLICA_UNEXPECTED],
        },
    },
]


@parametrize(*TEST_DATA)
def test_redisctl_wait_replica_synced(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts):
    main_test(capsys, argv, conn_func, expected_code, expected_out_parts, expected_err_parts)
