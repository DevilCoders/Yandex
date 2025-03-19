"""
SQLServer database modify tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.sqlserver.utils import get_sqlserver_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_sqlserver_database_modify_interrupt_consistency(mocker):
    """
    Check compute database modify interruptions
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

    args['target-database'] = 'test-database'

    check_task_interrupt_consistency(
        mocker,
        'sqlserver_database_modify',
        args,
        state,
    )


def test_sqlserver_database_modify_mlock_usage(mocker):
    """
    Check database modify mlock usage
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

    args['target-database'] = 'test-database'

    check_mlock_usage(
        mocker,
        'sqlserver_database_modify',
        args,
        state,
    )
