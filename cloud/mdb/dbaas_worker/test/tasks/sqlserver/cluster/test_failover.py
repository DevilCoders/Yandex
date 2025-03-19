"""
SQLServer cluster start failover tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.sqlserver.utils import get_sqlserver_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_sqlserver_cluster_start_failover_interrupt_consistency(mocker):
    """
    Check compute start failover interruptions
    """
    args = {
        'hosts': {
            'host1': get_sqlserver_compute_host(geo='geo1'),
            'host2': get_sqlserver_compute_host(geo='geo2'),
            'host3': get_sqlserver_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'sqlserver_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target_host'] = 'host1'
    check_task_interrupt_consistency(
        mocker,
        'sqlserver_cluster_start_failover',
        args,
        state,
    )


def test_sqlserver_cluster_start_failover_mlock_usage(mocker):
    """
    Check mlock usage
    """
    args = {
        'hosts': {
            'host1': get_sqlserver_compute_host(geo='geo1'),
            'host2': get_sqlserver_compute_host(geo='geo2'),
            'host3': get_sqlserver_compute_host(geo='geo3'),
        },
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'sqlserver_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['target_host'] = 'host1'
    check_mlock_usage(
        mocker,
        'sqlserver_cluster_start_failover',
        args,
        state,
    )
