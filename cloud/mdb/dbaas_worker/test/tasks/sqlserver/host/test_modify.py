"""
SQLServer host modify tests
"""

from test.mocks import checked_run_task_with_mocks
from test.tasks.sqlserver.utils import get_sqlserver_compute_host
from test.tasks.utils import check_mlock_usage, check_task_interrupt_consistency


def test_compute_sqlserver_host_modify_interrupt_consistency(mocker):
    """
    Check compute host modify interruptions
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

    args['host'] = {'fqdn': 'host3'}

    check_task_interrupt_consistency(
        mocker,
        'sqlserver_host_modify',
        args,
        state,
    )


def test_sqlserver_host_modify_mlock_usage(mocker):
    """
    Check host modify mlock usage
    """
    args = {
        'hosts': {
            'host1': get_sqlserver_compute_host(geo='geo1'),
            'host2': get_sqlserver_compute_host(geo='geo2'),
            'host3': get_sqlserver_compute_host(geo='geo3'),
        },
        'zk_hosts': 'test-zk',
    }
    *_, state = checked_run_task_with_mocks(
        mocker, 'sqlserver_cluster_create', dict(**args, s3_bucket='test-s3-bucket')
    )

    args['host'] = {'fqdn': 'host3'}

    check_mlock_usage(
        mocker,
        'sqlserver_host_modify',
        args,
        state,
    )
